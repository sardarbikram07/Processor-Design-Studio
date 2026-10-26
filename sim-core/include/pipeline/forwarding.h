#pragma once
#include "stages.h"
#include <cstdint>

// ─── Forwarding Unit ──────────────────────────────────────────────────────────
// Determines which pipeline stage to forward register values from,
// and provides the forwarded values for EX stage inputs.

enum class ForwardSrc {
    REGISTER_FILE,  // Use value from register file (no hazard)
    EX_MEM,         // Forward from EX/MEM ALU result
    MEM_WB,         // Forward from MEM/WB result
};

struct ForwardDecision {
    ForwardSrc rs_src = ForwardSrc::REGISTER_FILE;
    ForwardSrc rt_src = ForwardSrc::REGISTER_FILE;
};

class ForwardingUnit {
public:
    // Determine forwarding sources for rs and rt of the instruction in EX stage.
    static ForwardDecision decide(
        uint8_t       ex_rs,    // rs of instruction in EX
        uint8_t       ex_rt,    // rt of instruction in EX
        const EX_MEM& ex_mem,   // latch between EX and MEM
        const MEM_WB& mem_wb    // latch between MEM and WB
    );

    // Apply forwarding: return the actual operand value after forwarding.
    static int32_t apply(
        ForwardSrc    src,
        int32_t       reg_val,  // value from register file
        const EX_MEM& ex_mem,
        const MEM_WB& mem_wb
    );
};
