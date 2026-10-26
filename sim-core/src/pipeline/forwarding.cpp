#include "pipeline/forwarding.h"

ForwardDecision ForwardingUnit::decide(
    uint8_t       ex_rs,
    uint8_t       ex_rt,
    const EX_MEM& ex_mem,
    const MEM_WB& mem_wb)
{
    ForwardDecision dec;

    // ── rs forwarding ────────────────────────────────────────────────────────
    if (ex_rs != 0) {
        // EX/MEM → EX (higher priority, more recent)
        if (ex_mem.valid && !ex_mem.is_bubble &&
            ex_mem.reg_write && ex_mem.rd_dest == ex_rs) {
            dec.rs_src = ForwardSrc::EX_MEM;
        }
        // MEM/WB → EX
        else if (mem_wb.valid && !mem_wb.is_bubble &&
                 mem_wb.reg_write && mem_wb.rd_dest == ex_rs) {
            dec.rs_src = ForwardSrc::MEM_WB;
        }
    }

    // ── rt forwarding ────────────────────────────────────────────────────────
    if (ex_rt != 0) {
        if (ex_mem.valid && !ex_mem.is_bubble &&
            ex_mem.reg_write && ex_mem.rd_dest == ex_rt) {
            dec.rt_src = ForwardSrc::EX_MEM;
        }
        else if (mem_wb.valid && !mem_wb.is_bubble &&
                 mem_wb.reg_write && mem_wb.rd_dest == ex_rt) {
            dec.rt_src = ForwardSrc::MEM_WB;
        }
    }

    return dec;
}

int32_t ForwardingUnit::apply(
    ForwardSrc    src,
    int32_t       reg_val,
    const EX_MEM& ex_mem,
    const MEM_WB& mem_wb)
{
    switch (src) {
        case ForwardSrc::EX_MEM: return ex_mem.alu_result;
        case ForwardSrc::MEM_WB: return mem_wb.result;
        default:                 return reg_val;
    }
}
