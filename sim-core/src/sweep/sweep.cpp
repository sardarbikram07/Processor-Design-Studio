#include "sweep/sweep.h"
#include "assembler.h"
#include <chrono>
#include <stdexcept>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

SweepRunner::SweepRunner(SweepConfig sweep) : sweep_(std::move(sweep)) {}

std::vector<SimConfig> SweepRunner::build_configs() const {
    std::vector<SimConfig> configs;

    for (const auto& bench : sweep_.benchmarks) {
        for (const auto& pv : sweep_.pipeline_variants) {
            // For functional simulator, only use no-cache and no predictor
            // For pipelined, sweep all cache and predictor variants
            if (!pv.enabled) {
                SimConfig cfg;
                cfg.benchmark = bench;
                cfg.pipeline.enabled    = false;
                cfg.pipeline.forwarding = false;
                cfg.pipeline.stalling   = false;
                cfg.cache.sizeKB = 0;
                cfg.branchPredictor.type = "STATIC_NOT_TAKEN";
                configs.push_back(cfg);
            } else {
                for (const auto& cv : sweep_.cache_variants) {
                    for (const auto& pt : sweep_.predictor_types) {
                        SimConfig cfg;
                        cfg.benchmark = bench;
                        cfg.pipeline.enabled    = pv.enabled;
                        cfg.pipeline.forwarding = pv.forwarding;
                        cfg.pipeline.stalling   = pv.stalling;
                        cfg.cache.sizeKB       = cv.sizeKB;
                        cfg.cache.blockSize    = cv.blockSize;
                        cfg.cache.associativity = cv.assoc;
                        cfg.cache.replacement  = cv.replacement;
                        cfg.branchPredictor.type = pt;
                        configs.push_back(cfg);
                    }
                }
            }
        }
    }
    return configs;
}

size_t SweepRunner::config_count() const {
    return build_configs().size();
}

SweepEntry SweepRunner::run_one(const SimConfig& cfg) const {
    SweepEntry entry;
    entry.config = cfg;

    // Assemble the benchmark
    std::string asm_path = sweep_.benchmarks_dir + "/" + cfg.benchmark + ".asm";
    std::vector<uint32_t> program;
    try {
        program = assemble_file(asm_path);
    } catch (const std::exception& e) {
        // Mark as failed — return empty result
        entry.result.config = cfg;
        return entry;
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    if (!cfg.pipeline.enabled) {
        Simulator sim(cfg);
        sim.load_program(program);
        entry.result = sim.run();
    } else {
        PipelinedSimulator sim(cfg);
        sim.load_program(program);
        entry.result = sim.run();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    entry.wall_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    return entry;
}

std::vector<SweepEntry> SweepRunner::run_sequential() {
    auto configs = build_configs();
    std::vector<SweepEntry> results;
    results.reserve(configs.size());
    for (const auto& cfg : configs) {
        results.push_back(run_one(cfg));
    }
    return results;
}

std::vector<SweepEntry> SweepRunner::run_parallel(int threads) {
    auto configs = build_configs();
    std::vector<SweepEntry> results(configs.size());

#ifdef _OPENMP
    if (threads > 0) omp_set_num_threads(threads);
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < static_cast<int>(configs.size()); ++i) {
        results[i] = run_one(configs[i]);
    }
#else
    // Fallback: sequential
    for (size_t i = 0; i < configs.size(); ++i) {
        results[i] = run_one(configs[i]);
    }
#endif

    return results;
}
