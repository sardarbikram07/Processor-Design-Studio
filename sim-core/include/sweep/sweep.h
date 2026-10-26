#pragma once
#include "simulator.h"
#include "pipelined_simulator.h"
#include <vector>
#include <string>
#include <functional>

// ─── Sweep Configuration ──────────────────────────────────────────────────────
// Defines the "grid" of values to sweep over for each config dimension.
struct SweepConfig {
    std::vector<std::string> benchmarks = {"loop_sum", "factorial", "bubble_sort", "array_sum", "hazard_heavy", "matrix_multiply"};

    // Pipeline variants
    struct PipelineVariant { bool enabled; bool forwarding; bool stalling; std::string label; };
    std::vector<PipelineVariant> pipeline_variants = {
        {false, false, false,  "functional"},
        {true,  true,  true,   "pipeline+fwd"},
        {true,  false, true,   "pipeline+stall"},
    };

    // Cache variants (sizeKB=0 means disabled)
    struct CacheVariant { int sizeKB; int blockSize; int assoc; std::string replacement; std::string label; };
    std::vector<CacheVariant> cache_variants = {
        {0,    64, 1,  "LRU",    "no-cache"},
        {1,    16, 1,  "LRU",    "1KB-DM-LRU"},
        {4,    64, 2,  "LRU",    "4KB-2way-LRU"},
        {4,    64, 4,  "LRU",    "4KB-4way-LRU"},
        {4,    64, 4,  "FIFO",   "4KB-4way-FIFO"},
        {4,    64, 4,  "Random", "4KB-4way-Rand"},
    };

    // Branch predictor variants
    std::vector<std::string> predictor_types = {
        "STATIC_NOT_TAKEN", "STATIC_TAKEN", "ONE_BIT", "TWO_BIT_SATURATING"
    };

    // Optional: directory containing .asm benchmark files
    std::string benchmarks_dir = "sim-core/benchmarks";

    // Optional: number of OpenMP threads (0 = auto)
    int omp_threads = 0;
};

// ─── Single sweep result entry ────────────────────────────────────────────────
struct SweepEntry {
    SimConfig   config;
    SimResult   result;
    double      wall_time_ms = 0.0; // time to run this single sim
};

// ─── Sweep Runner ─────────────────────────────────────────────────────────────
class SweepRunner {
public:
    explicit SweepRunner(SweepConfig sweep);

    // Run all configurations sequentially.
    std::vector<SweepEntry> run_sequential();

    // Run all configurations in parallel using OpenMP.
    // Returns the same results as run_sequential, but potentially much faster.
    std::vector<SweepEntry> run_parallel(int threads = 0);

    // Total number of configurations in the sweep
    size_t config_count() const;

    // Get all SimConfigs without running (for scheduling)
    std::vector<SimConfig> build_configs() const;

private:
    SweepConfig sweep_;
    SweepEntry run_one(const SimConfig& cfg) const;
};
