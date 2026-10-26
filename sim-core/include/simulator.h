#pragma once
#include "regfile.h"
#include "memory.h"
#include "isa.h"
#include "cache/cache.h"
#include <cstdint>
#include <vector>
#include <memory>

// ─── Simulation Configuration ────────────────────────────────────────────────
struct PipelineConfig {
    bool enabled    = false;
    bool forwarding = true;
    bool stalling   = true;
};

struct CacheConfig {
    int    sizeKB       = 0;    // 0 = no cache (direct to memory)
    int    blockSize    = 64;
    int    associativity = 1;
    std::string replacement = "LRU";
    std::string writePolicy = "write-through";
};

struct PredictorConfig {
    std::string type = "STATIC_NOT_TAKEN";
};

struct SimConfig {
    std::string     benchmark;
    PipelineConfig  pipeline;
    CacheConfig     cache;
    PredictorConfig branchPredictor;
};

// ─── Simulation Result ────────────────────────────────────────────────────────
struct SimResult {
    SimConfig   config;
    uint64_t    totalCycles           = 0;
    uint64_t    instructionsExecuted  = 0;
    double      cpi                   = 0.0;
    double      ipc                   = 0.0;
    double      cacheHitRate          = 0.0;
    double      branchMispredictionRate = 0.0;
    uint64_t    stallCycles           = 0;
};

// ─── Functional Simulator ────────────────────────────────────────────────────
// Runs a single-cycle fetch-decode-execute loop (no pipeline in Phase 1).
class Simulator {
public:
    explicit Simulator(SimConfig cfg);

    // Load an assembled program (vector of 32-bit instruction words) into memory starting at word 0.
    void load_program(const std::vector<uint32_t>& program);

    // Load data words into memory at a given byte offset (after the program area).
    void load_data(uint32_t byte_offset, const std::vector<uint32_t>& data);

    // Run until HALT or max_cycles reached. Returns the SimResult.
    SimResult run(uint64_t max_cycles = 10'000'000);

    // Access internals (for unit tests).
    const RegisterFile& regs() const { return regs_; }
    const Memory&       mem()  const { return mem_;  }

    static constexpr uint32_t DATA_SECTION_BASE = 0x400; // 1KB offset for data

private:
    SimConfig    cfg_;
    RegisterFile regs_;
    Memory       mem_;
    uint32_t     pc_ = 0;
    bool         halted_ = false;
    std::unique_ptr<Cache> cache_; // null if cache disabled

    void execute_one(const Instruction& instr, SimResult& result);
    // Cache-aware memory access helpers
    uint32_t mem_load(uint32_t addr, SimResult& result);
    void     mem_store(uint32_t addr, uint32_t val, SimResult& result);
};
