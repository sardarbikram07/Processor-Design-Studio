#pragma once
#include "simulator.h"
#include "pipeline/stages.h"
#include "predictor/predictor.h"
#include <cstdint>
#include <memory>

// 5-stage pipelined simulator: IF → ID → EX → MEM → WB
// Respects SimConfig.pipeline.forwarding and SimConfig.pipeline.stalling flags.
// Falls back to functional simulator behavior when pipeline.enabled = false.
class PipelinedSimulator {
public:
    explicit PipelinedSimulator(SimConfig cfg);

    void load_program(const std::vector<uint32_t>& program);
    void load_data(uint32_t byte_offset, const std::vector<uint32_t>& data);

    SimResult run(uint64_t max_cycles = 10'000'000);

    const RegisterFile& regs() const { return regs_; }
    const Memory&       mem()  const { return mem_;  }

private:
    SimConfig    cfg_;
    RegisterFile regs_;
    Memory       mem_;
    uint32_t     pc_       = 0;
    bool         halted_   = false;
    uint32_t     prog_end_ = 0;
    std::unique_ptr<Cache>           cache_;
    std::unique_ptr<BranchPredictor> predictor_;

    PipelineState pipeline_;

    // ── Stage functions ────────────────────────────────────────────────────
    void stage_IF (bool stall, SimResult& result);
    void stage_ID (bool stall, bool flush, SimResult& result);
    void stage_EX (SimResult& result);
    void stage_MEM(SimResult& result);
    void stage_WB (SimResult& result);

    // ── Hazard / forwarding helpers ────────────────────────────────────────
    bool detect_stall(uint8_t rs, uint8_t rt) const;

    int32_t alu_compute(const ID_EX& id_ex, int32_t a, int32_t b) const;
    uint8_t  get_rd_dest(const ID_EX& id_ex) const;
    bool     does_reg_write(const ID_EX& id_ex) const;
};
