#include "pipelined_simulator.h"
#include "decoder.h"
#include "pipeline/hazard.h"
#include "pipeline/forwarding.h"
#include <stdexcept>

PipelinedSimulator::PipelinedSimulator(SimConfig cfg)
    : cfg_(std::move(cfg)), mem_(Memory::DEFAULT_SIZE_BYTES)
{
    // Init cache
    if (cfg_.cache.sizeKB > 0) {
        cache_ = std::make_unique<Cache>(
            static_cast<uint32_t>(cfg_.cache.sizeKB) * 1024,
            static_cast<uint32_t>(cfg_.cache.blockSize),
            static_cast<uint32_t>(cfg_.cache.associativity),
            Cache::parse_policy(cfg_.cache.replacement)
        );
    }
    // Init branch predictor
    predictor_.reset(BranchPredictor::create(cfg_.branchPredictor.type));
}


void PipelinedSimulator::load_program(const std::vector<uint32_t>& program) {
    for (uint32_t i = 0; i < program.size(); ++i)
        mem_.write_instruction(i, program[i]);
    prog_end_ = static_cast<uint32_t>(program.size()) * 4;
    pc_       = 0;
    halted_   = false;
    regs_.reset();
    pipeline_ = PipelineState{};
}

void PipelinedSimulator::load_data(uint32_t byte_offset, const std::vector<uint32_t>& data) {
    for (uint32_t i = 0; i < data.size(); ++i)
        mem_.store_word(byte_offset + i * 4, data[i]);
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
static bool is_bubble(const ID_EX& s)  { return !s.valid  || s.is_bubble; }
static bool is_bubble(const EX_MEM& s) { return !s.valid  || s.is_bubble; }
static bool is_bubble(const MEM_WB& s) { return !s.valid  || s.is_bubble; }

int32_t PipelinedSimulator::alu_compute(const ID_EX& s, int32_t a, int32_t b) const {
    switch (s.opcode) {
        case Opcode::RTYPE:
            switch (s.funct) {
                case Funct::ADD: return a + b;
                case Funct::SUB: return a - b;
                case Funct::AND: return a & b;
                case Funct::OR:  return a | b;
                case Funct::XOR: return a ^ b;
                case Funct::SLL: return b << s.shamt;
                case Funct::SRL: return static_cast<int32_t>(
                    static_cast<uint32_t>(b) >> s.shamt);
                case Funct::JR:  return 0;
                default:         return 0;
            }
        case Opcode::ADDI: return a + s.imm;
        case Opcode::ANDI: return a & s.imm;
        case Opcode::ORI:  return a | s.imm;
        case Opcode::LW:
        case Opcode::SW:   return a + s.imm;
        case Opcode::BEQ:  return (a == b) ? 1 : 0;
        case Opcode::BNE:  return (a != b) ? 1 : 0;
        case Opcode::BLT:  return (a <  b) ? 1 : 0;
        case Opcode::BGE:  return (a >= b) ? 1 : 0;
        default:           return 0;
    }
}

uint8_t PipelinedSimulator::get_rd_dest(const ID_EX& s) const {
    switch (s.opcode) {
        case Opcode::RTYPE: return (s.funct == Funct::JR) ? 0 : s.rd;
        case Opcode::ADDI: case Opcode::ANDI: case Opcode::ORI: case Opcode::LW:
            return s.rt;
        case Opcode::JAL:   return 15;
        default:            return 0;
    }
}

bool PipelinedSimulator::does_reg_write(const ID_EX& s) const {
    switch (s.opcode) {
        case Opcode::RTYPE:    return s.funct != Funct::JR && s.rd != 0;
        case Opcode::ADDI: case Opcode::ANDI: case Opcode::ORI: case Opcode::LW:
            return s.rt != 0;
        case Opcode::JAL:      return true;
        default:               return false;
    }
}

// ─── Stall detection ──────────────────────────────────────────────────────────
// Uses CURRENT pipeline state (not the "next" cycle) to decide if a stall
// is needed for the instruction sitting in IF/ID.
bool PipelinedSimulator::detect_stall(uint8_t rs, uint8_t rt) const {
    const auto& id_ex  = pipeline_.id_ex;
    const auto& ex_mem = pipeline_.ex_mem;

    // Load-use: always stall (even with forwarding, 1-cycle delay needed)
    if (!is_bubble(id_ex) && id_ex.opcode == Opcode::LW) {
        uint8_t ld = id_ex.rt;
        if (ld != 0 && (ld == rs || ld == rt)) return true;
    }

    // RAW without forwarding: stall for EX-stage and MEM-stage producers
    if (cfg_.pipeline.stalling && !cfg_.pipeline.forwarding) {
        if (!is_bubble(id_ex) && does_reg_write(id_ex)) {
            uint8_t dest = get_rd_dest(id_ex);
            if (dest != 0 && (dest == rs || dest == rt)) return true;
        }
        if (!is_bubble(ex_mem) && ex_mem.reg_write) {
            if (ex_mem.rd_dest != 0 &&
                (ex_mem.rd_dest == rs || ex_mem.rd_dest == rt)) return true;
        }
    }
    return false;
}

// ─── Main run loop (latch-snapshot based) ─────────────────────────────────────
// All stages read from the CURRENT latches (snapshot) and write into NEW latches.
// Only at the end of the cycle do we commit the new latches.
// This correctly models simultaneous register updates and avoids read-after-write
// within a single simulated clock cycle.
SimResult PipelinedSimulator::run(uint64_t max_cycles) {
    SimResult result;
    result.config = cfg_;

    uint64_t total_branches = 0;
    uint64_t mispredictions = 0;

    while (result.totalCycles < max_cycles) {
        // ── Snapshot current pipeline state (all stages read from these) ───
        IF_ID   cur_if_id  = pipeline_.if_id;
        ID_EX   cur_id_ex  = pipeline_.id_ex;
        EX_MEM  cur_ex_mem = pipeline_.ex_mem;
        MEM_WB  cur_mem_wb = pipeline_.mem_wb;

        // ── Next-cycle latch values ────────────────────────────────────────
        IF_ID   next_if_id;
        ID_EX   next_id_ex;
        EX_MEM  next_ex_mem;
        MEM_WB  next_mem_wb;

        // ── Determine stall (based on current latches) ─────────────────────
        uint8_t if_rs = 0, if_rt = 0;
        if (cur_if_id.valid) {
            uint32_t raw = cur_if_id.instruction;
            Opcode op = static_cast<Opcode>((raw >> 26) & 0x3F);
            if (op != Opcode::J && op != Opcode::JAL &&
                op != Opcode::NOP && op != Opcode::HALT) {
                if_rs = (raw >> 22) & 0xF;
                if_rt = (raw >> 18) & 0xF;
            }
        }
        bool stall = !halted_ && cfg_.pipeline.stalling &&
                     detect_stall(if_rs, if_rt);
        if (stall) result.stallCycles++;

        // ── Track branches (for misprediction rate) ────────────────────────
        if (!is_bubble(cur_id_ex)) {
            Opcode op = cur_id_ex.opcode;
            if (op == Opcode::BEQ || op == Opcode::BNE ||
                op == Opcode::BLT || op == Opcode::BGE) {
                total_branches++;
            }
        }

        // ══ WB Stage: commit instruction from MEM/WB ══════════════════════
        if (!is_bubble(cur_mem_wb)) {
            if (cur_mem_wb.reg_write && cur_mem_wb.rd_dest != 0) {
                regs_.write(cur_mem_wb.rd_dest, cur_mem_wb.result);
            }
            result.instructionsExecuted++;
        }
        // MEM/WB is consumed — next cycle MEM will fill it
        next_mem_wb.valid     = false;
        next_mem_wb.is_bubble = true;

        // ══ MEM Stage: from EX/MEM → produce MEM/WB ═══════════════════════
        if (!is_bubble(cur_ex_mem)) {
            next_mem_wb.pc        = cur_ex_mem.pc;
            next_mem_wb.opcode    = cur_ex_mem.opcode;
            next_mem_wb.rd_dest   = cur_ex_mem.rd_dest;
            next_mem_wb.reg_write = cur_ex_mem.reg_write;
            next_mem_wb.valid     = true;
            next_mem_wb.is_bubble = false;

            if (cur_ex_mem.mem_read) {
                next_mem_wb.result = static_cast<int32_t>(
                    mem_.load_word(static_cast<uint32_t>(cur_ex_mem.alu_result)));
            } else if (cur_ex_mem.mem_write) {
                mem_.store_word(static_cast<uint32_t>(cur_ex_mem.alu_result),
                                static_cast<uint32_t>(cur_ex_mem.rt_val));
                next_mem_wb.reg_write = false;
                next_mem_wb.result    = 0;
            } else {
                next_mem_wb.result = cur_ex_mem.alu_result;
            }
        }

        // ══ EX Stage: from ID/EX → produce EX/MEM ═════════════════════════
        bool branch_taken   = false;
        uint32_t branch_target = 0;

        if (!is_bubble(cur_id_ex)) {
            const ID_EX& s = cur_id_ex;

            // Forwarding
            int32_t a = s.rs_val;
            int32_t b = s.rt_val;
            if (cfg_.pipeline.forwarding) {
                auto dec = ForwardingUnit::decide(s.rs, s.rt, cur_ex_mem, cur_mem_wb);
                a = ForwardingUnit::apply(dec.rs_src, a, cur_ex_mem, cur_mem_wb);
                b = ForwardingUnit::apply(dec.rt_src, b, cur_ex_mem, cur_mem_wb);
            }

            int32_t alu = alu_compute(s, a, b);

            next_ex_mem.pc         = s.pc;
            next_ex_mem.opcode     = s.opcode;
            next_ex_mem.rt         = s.rt;
            next_ex_mem.rd_dest    = get_rd_dest(s);
            next_ex_mem.alu_result = alu;
            next_ex_mem.rt_val     = b;
            next_ex_mem.mem_read   = (s.opcode == Opcode::LW);
            next_ex_mem.mem_write  = (s.opcode == Opcode::SW);
            next_ex_mem.reg_write  = does_reg_write(s);
            next_ex_mem.valid      = true;
            next_ex_mem.is_bubble  = false;

            switch (s.opcode) {
                case Opcode::BEQ: case Opcode::BNE:
                case Opcode::BLT: case Opcode::BGE: {
                    next_ex_mem.reg_write = false;
                    total_branches++;
                    bool actual_taken = (alu != 0);
                    uint32_t actual_target = static_cast<uint32_t>(
                        static_cast<int32_t>(s.pc + 4) + s.imm * 4);

                    // Predictor: check prediction, then update with actual outcome
                    bool prediction = predictor_->predict(s.pc);
                    predictor_->update(s.pc, actual_taken);
                    if (prediction != actual_taken) mispredictions++;

                    // Always-not-taken speculation: flush when taken
                    if (actual_taken) {
                        branch_taken  = true;
                        branch_target = actual_target;
                    }
                    break;
                }
                case Opcode::J:
                    branch_taken  = true;
                    branch_target = s.addr * 4;
                    next_ex_mem.reg_write = false;
                    break;
                case Opcode::JAL:
                    branch_taken           = true;
                    branch_target          = s.addr * 4;
                    next_ex_mem.alu_result = static_cast<int32_t>(s.pc + 4);
                    next_ex_mem.rd_dest    = 15;
                    next_ex_mem.reg_write  = true;
                    break;
                case Opcode::RTYPE:
                    if (s.funct == Funct::JR) {
                        branch_taken  = true;
                        branch_target = static_cast<uint32_t>(a);
                        next_ex_mem.reg_write = false;
                    }
                    break;
                case Opcode::HALT:
                    halted_               = true;
                    next_ex_mem.valid     = false;
                    next_ex_mem.is_bubble = true;
                    break;
                default: break;
            }
        } else {
            next_ex_mem.valid     = false;
            next_ex_mem.is_bubble = true;
        }

        // ══ Determine if we need to flush/redirect ═══════════════════════════════════════════════════════
        // branch_taken=true  → flush the 2 speculatively-fetched wrong-path instructions
        // For unconditional jumps and conditional-taken, both need a redirect.
        bool need_redirect = branch_taken;
        uint32_t corrected_pc = branch_target;
        // Note: branch-flush cycles are implicit in totalCycles (2 bubble cycles).
        // stallCycles tracks only data-hazard stalls, not control penalties.

        // ══ ID Stage ══════════════════════════════════════════════════════
        if (need_redirect) {
            next_id_ex.valid     = false;
            next_id_ex.is_bubble = true;
            next_if_id.valid     = false;
            pc_ = corrected_pc;
        } else if (stall) {
            next_if_id = cur_if_id;
            next_id_ex.valid     = false;
            next_id_ex.is_bubble = true;
        } else if (cur_if_id.valid && !halted_) {
            uint32_t raw    = cur_if_id.instruction;
            Instruction ins = decode(raw);

            next_id_ex.pc        = cur_if_id.pc;
            next_id_ex.opcode    = ins.opcode;
            next_id_ex.funct     = ins.funct;
            next_id_ex.format    = ins.format;
            next_id_ex.rs        = ins.rs;
            next_id_ex.rt        = ins.rt;
            next_id_ex.rd        = ins.rd;
            next_id_ex.shamt     = ins.shamt;
            next_id_ex.imm       = ins.imm;
            next_id_ex.addr      = ins.addr;
            next_id_ex.rs_val    = regs_.read(ins.rs);
            next_id_ex.rt_val    = regs_.read(ins.rt);
            next_id_ex.valid     = true;
            next_id_ex.is_bubble = false;
            next_if_id.valid     = false;
        } else {
            next_id_ex.valid     = false;
            next_id_ex.is_bubble = true;
            next_if_id.valid     = false;
        }

        // ══ IF Stage ══════════════════════════════════════════════════════
        if (!need_redirect && !stall && !halted_ && pc_ < mem_.size_bytes()) {
            next_if_id.pc          = pc_;
            next_if_id.instruction = mem_.load_word(pc_);
            next_if_id.valid       = true;
            pc_ += 4;
        }

        // ══ Commit latches ═════════════════════════════════════════════════
        pipeline_.if_id  = next_if_id;
        pipeline_.id_ex  = next_id_ex;
        pipeline_.ex_mem = next_ex_mem;
        pipeline_.mem_wb = next_mem_wb;

        result.totalCycles++;

        if (halted_) {
            bool empty =
                !pipeline_.if_id.valid  &&
                is_bubble(pipeline_.id_ex)  &&
                is_bubble(pipeline_.ex_mem) &&
                is_bubble(pipeline_.mem_wb);
            if (empty) break;
        }
    }

    // Finalize metrics
    if (result.instructionsExecuted > 0) {
        result.cpi = static_cast<double>(result.totalCycles) /
                     static_cast<double>(result.instructionsExecuted);
        result.ipc = 1.0 / result.cpi;
    }
    if (cache_) result.cacheHitRate = cache_->stats().hit_rate();
    result.branchMispredictionRate =
        (total_branches > 0)
        ? static_cast<double>(mispredictions) / static_cast<double>(total_branches)
        : 0.0;

    return result;
}

void PipelinedSimulator::stage_IF(bool, SimResult&)        {}
void PipelinedSimulator::stage_ID(bool, bool, SimResult&)  {}
void PipelinedSimulator::stage_EX(SimResult&)              {}
void PipelinedSimulator::stage_MEM(SimResult&)             {}
void PipelinedSimulator::stage_WB(SimResult&)              {}
