#include "assembler.h"
#include "simulator.h"
#include "pipelined_simulator.h"
#include "sweep/sweep.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdlib>

// ─── Minimal JSON helpers (no external deps) ─────────────────────────────────
static std::string json_get_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t kpos = json.find(search);
    if (kpos == std::string::npos) return "";
    size_t colon = json.find(':', kpos + search.size());
    if (colon == std::string::npos) return "";
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

static bool json_get_bool(const std::string& json, const std::string& key, bool default_val) {
    std::string search = "\"" + key + "\"";
    size_t kpos = json.find(search);
    if (kpos == std::string::npos) return default_val;
    size_t colon = json.find(':', kpos + search.size());
    if (colon == std::string::npos) return default_val;
    size_t val_start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (val_start == std::string::npos) return default_val;
    return json.substr(val_start, 4) == "true";
}

static int json_get_int(const std::string& json, const std::string& key, int default_val) {
    std::string search = "\"" + key + "\"";
    size_t kpos = json.find(search);
    if (kpos == std::string::npos) return default_val;
    size_t colon = json.find(':', kpos + search.size());
    if (colon == std::string::npos) return default_val;
    size_t val_start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (val_start == std::string::npos) return default_val;
    return std::stoi(json.substr(val_start));
}

static std::string json_get_object(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t kpos = json.find(search);
    if (kpos == std::string::npos) return "{}";
    size_t brace = json.find('{', kpos + search.size());
    if (brace == std::string::npos) return "{}";
    int depth = 0;
    for (size_t i = brace; i < json.size(); ++i) {
        if (json[i] == '{') depth++;
        else if (json[i] == '}') { depth--; if (depth == 0) return json.substr(brace, i - brace + 1); }
    }
    return "{}";
}

// ─── JSON output helpers ──────────────────────────────────────────────────────
static std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static std::string result_to_json(const SimResult& r, double wall_ms = -1.0) {
    std::ostringstream o;
    o << "{\n"
      << "  \"benchmark\": \""         << escape_json(r.config.benchmark) << "\",\n"
      << "  \"pipeline\": { \"enabled\": " << (r.config.pipeline.enabled ? "true" : "false")
      << ", \"forwarding\": " << (r.config.pipeline.forwarding ? "true" : "false")
      << ", \"stalling\": "   << (r.config.pipeline.stalling   ? "true" : "false") << " },\n"
      << "  \"cache\": { \"sizeKB\": "    << r.config.cache.sizeKB
      << ", \"blockSize\": "   << r.config.cache.blockSize
      << ", \"associativity\": " << r.config.cache.associativity
      << ", \"replacement\": \"" << escape_json(r.config.cache.replacement) << "\" },\n"
      << "  \"branchPredictor\": { \"type\": \"" << escape_json(r.config.branchPredictor.type) << "\" },\n"
      << "  \"totalCycles\": "             << r.totalCycles             << ",\n"
      << "  \"instructionsExecuted\": "    << r.instructionsExecuted    << ",\n"
      << "  \"cpi\": "                     << r.cpi                     << ",\n"
      << "  \"ipc\": "                     << r.ipc                     << ",\n"
      << "  \"cacheHitRate\": "            << r.cacheHitRate            << ",\n"
      << "  \"branchMispredictionRate\": " << r.branchMispredictionRate << ",\n"
      << "  \"stallCycles\": "             << r.stallCycles;
    if (wall_ms >= 0.0)
        o << ",\n  \"wallTimeMs\": " << wall_ms;
    o << "\n}";
    return o.str();
}

// Parse a SimConfig from JSON string
static SimConfig parse_sim_config(const std::string& json_input) {
    SimConfig cfg;
    cfg.benchmark = json_get_string(json_input, "benchmark");

    std::string pipeline_obj = json_get_object(json_input, "pipeline");
    cfg.pipeline.enabled    = json_get_bool(pipeline_obj, "enabled",    false);
    cfg.pipeline.forwarding = json_get_bool(pipeline_obj, "forwarding", true);
    cfg.pipeline.stalling   = json_get_bool(pipeline_obj, "stalling",   true);

    std::string cache_obj = json_get_object(json_input, "cache");
    cfg.cache.sizeKB        = json_get_int   (cache_obj, "sizeKB",        0);
    cfg.cache.blockSize     = json_get_int   (cache_obj, "blockSize",     64);
    cfg.cache.associativity = json_get_int   (cache_obj, "associativity", 1);
    cfg.cache.replacement   = json_get_string(cache_obj, "replacement");
    cfg.cache.writePolicy   = json_get_string(cache_obj, "writePolicy");

    std::string pred_obj = json_get_object(json_input, "branchPredictor");
    cfg.branchPredictor.type = json_get_string(pred_obj, "type");
    if (cfg.branchPredictor.type.empty()) cfg.branchPredictor.type = "STATIC_NOT_TAKEN";

    return cfg;
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string json_input;
    {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        json_input = ss.str();
    }

    std::string bench_dir = "sim-core/benchmarks";
    int omp_threads = 0;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--benchmarks-dir") bench_dir = argv[i + 1];
        if (std::string(argv[i]) == "--threads")        omp_threads = std::stoi(argv[i + 1]);
    }

    // Detect mode
    std::string mode = json_get_string(json_input, "mode");

    if (mode == "sweep") {
        // ── SWEEP MODE ──────────────────────────────────────────────────────
        SweepConfig sweep;
        sweep.benchmarks_dir = bench_dir;
        sweep.omp_threads    = omp_threads;

        // Optional: override thread count from JSON
        std::string threads_str = json_get_string(json_input, "threads");
        if (!threads_str.empty()) {
            try { sweep.omp_threads = std::stoi(threads_str); } catch(...) {}
        }

        bool parallel = json_get_bool(json_input, "parallel", false);
        SweepRunner runner(sweep);
        std::vector<SweepEntry> results;

        if (parallel) {
            results = runner.run_parallel(sweep.omp_threads);
        } else {
            results = runner.run_sequential();
        }

        // Emit as JSON array
        std::cout << "[\n";
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << result_to_json(results[i].result, results[i].wall_time_ms);
            if (i + 1 < results.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";

    } else {
        // ── SINGLE RUN MODE ─────────────────────────────────────────────────
        SimConfig cfg = parse_sim_config(json_input);
        std::string asm_path = bench_dir + "/" + cfg.benchmark + ".asm";

        std::vector<uint32_t> program;
        try {
            program = assemble_file(asm_path);
        } catch (const std::exception& e) {
            std::cerr << "Assembler error: " << e.what() << "\n";
            return 1;
        }

        SimResult result;
        try {
            if (cfg.pipeline.enabled) {
                PipelinedSimulator sim(cfg);
                sim.load_program(program);
                result = sim.run();
            } else {
                Simulator sim(cfg);
                sim.load_program(program);
                result = sim.run();
            }
        } catch (const std::exception& e) {
            std::cerr << "Simulation error: " << e.what() << "\n";
            return 1;
        }

        // Emit as flat JSON — same schema as each sweep entry
        std::cout << result_to_json(result) << "\n";

    }

    return 0;
}
