#include "pipeline/hazard.h"

// A register "writes" if it's a destination and not $r0.
static bool writes_reg(const ID_EX& s, uint8_t reg) {
    if (reg == 0) return false;
    switch (s.opcode) {
        case Opcode::RTYPE:
            // JR doesn't write a register destination
            return (s.funct != Funct::JR) && (s.rd == reg);
        case Opcode::ADDI: case Opcode::ANDI: case Opcode::ORI:
        case Opcode::LW:
            return s.rt == reg;
        case Opcode::JAL:
            return reg == 15; // writes $r15
        default:
            return false;
    }
}

static bool ex_mem_writes_reg(const EX_MEM& s, uint8_t reg) {
    return s.reg_write && s.rd_dest == reg && reg != 0;
}

static bool mem_wb_writes_reg(const MEM_WB& s, uint8_t reg) {
    return s.reg_write && s.rd_dest == reg && reg != 0;
}

static bool is_load(const ID_EX& s) {
    return s.opcode == Opcode::LW;
}

HazardInfo HazardDetector::detect(
    const ID_EX&  id_ex,
    const EX_MEM& ex_mem,
    const IF_ID&  /*if_id*/,
    uint8_t       if_id_rs,
    uint8_t       if_id_rt,
    bool          stalling_enabled,
    bool          forwarding_enabled)
{
    HazardInfo info;
    (void)ex_mem; // used for structural checks in future phases

    if (!stalling_enabled && !forwarding_enabled) return info; // no protection

    // ── Load-Use Hazard ──────────────────────────────────────────────────────
    // LW in EX, instruction in ID reads the loaded register.
    // Cannot be resolved by forwarding alone — always needs a stall bubble.
    if (id_ex.valid && !id_ex.is_bubble && is_load(id_ex)) {
        uint8_t load_dest = id_ex.rt; // LW writes rt
        if (load_dest != 0 &&
            (load_dest == if_id_rs || load_dest == if_id_rt)) {
            if (stalling_enabled) {
                info.stall_needed = true;
                return info;
            }
        }
    }

    // ── RAW Hazard (stalling without forwarding) ─────────────────────────────
    // If forwarding is off, any back-to-back RAW needs stalls (2 cycles each).
    if (stalling_enabled && !forwarding_enabled) {
        // Check EX-stage destination against ID-stage sources
        if (id_ex.valid && !id_ex.is_bubble) {
            if (writes_reg(id_ex, if_id_rs) || writes_reg(id_ex, if_id_rt)) {
                info.stall_needed = true;
                return info;
            }
        }
        // Check MEM-stage destination against ID-stage sources
        if (ex_mem.valid && !ex_mem.is_bubble) {
            if (ex_mem_writes_reg(ex_mem, if_id_rs) ||
                ex_mem_writes_reg(ex_mem, if_id_rt)) {
                info.stall_needed = true;
                return info;
            }
        }
    }

    return info;
}
