#include "simulator.h"
#include "decoder.h"
#include <stdexcept>
#include <cstring>

Simulator::Simulator(SimConfig cfg) : cfg_(std::move(cfg)), mem_(Memory::DEFAULT_SIZE_BYTES) {
    // Initialize cache if configured
    if (cfg_.cache.sizeKB > 0) {
        uint32_t size_bytes = static_cast<uint32_t>(cfg_.cache.sizeKB) * 1024;
        cache_ = std::make_unique<Cache>(
            size_bytes,
            static_cast<uint32_t>(cfg_.cache.blockSize),
            static_cast<uint32_t>(cfg_.cache.associativity),
            Cache::parse_policy(cfg_.cache.replacement)
        );
    }
}

void Simulator::load_program(const std::vector<uint32_t>& program) {
    for (uint32_t i = 0; i < program.size(); ++i)
        mem_.write_instruction(i, program[i]);
    pc_      = 0;
    halted_  = false;
    regs_.reset();
}

void Simulator::load_data(uint32_t byte_offset, const std::vector<uint32_t>& data) {
    for (uint32_t i = 0; i < data.size(); ++i)
        mem_.store_word(byte_offset + i * 4, data[i]);
}

SimResult Simulator::run(uint64_t max_cycles) {
    SimResult result;
    result.config = cfg_;

    while (!halted_ && result.totalCycles < max_cycles) {
        uint32_t word = mem_.load_word(pc_);
        Instruction instr = decode(word);
        pc_ += 4;
        result.totalCycles++;
        execute_one(instr, result);
    }

    if (result.instructionsExecuted > 0) {
        result.cpi = static_cast<double>(result.totalCycles) /
                     static_cast<double>(result.instructionsExecuted);
        result.ipc = 1.0 / result.cpi;
    }
    if (cache_) {
        result.cacheHitRate = cache_->stats().hit_rate();
    }
    return result;
}

// Cache-aware memory helpers
uint32_t Simulator::mem_load(uint32_t addr, SimResult&) {
    if (cache_) cache_->access(addr);
    return mem_.load_word(addr);
}

void Simulator::mem_store(uint32_t addr, uint32_t val, SimResult&) {
    if (cache_) cache_->store(addr);
    mem_.store_word(addr, val);
}

void Simulator::execute_one(const Instruction& instr, SimResult& result) {
    using O = Opcode;
    using F = Funct;

    switch (instr.opcode) {
        // ── HALT ────────────────────────────────────────────────────────────
        case O::HALT:
            halted_ = true;
            result.instructionsExecuted++;
            return;

        // ── NOP ─────────────────────────────────────────────────────────────
        case O::NOP:
            result.instructionsExecuted++;
            return;

        // ── R-type ──────────────────────────────────────────────────────────
        case O::RTYPE: {
            int32_t rs_val = regs_.read(instr.rs);
            int32_t rt_val = regs_.read(instr.rt);
            switch (instr.funct) {
                case F::ADD: regs_.write(instr.rd, rs_val + rt_val);         break;
                case F::SUB: regs_.write(instr.rd, rs_val - rt_val);         break;
                case F::AND: regs_.write(instr.rd, rs_val & rt_val);         break;
                case F::OR:  regs_.write(instr.rd, rs_val | rt_val);         break;
                case F::XOR: regs_.write(instr.rd, rs_val ^ rt_val);         break;
                case F::SLL: regs_.write(instr.rd, rt_val << instr.shamt);   break;
                case F::SRL: regs_.write(instr.rd,
                    static_cast<int32_t>(static_cast<uint32_t>(rt_val) >> instr.shamt)); break;
                case F::JR:
                    pc_ = static_cast<uint32_t>(rs_val);
                    break;
                default:
                    throw std::runtime_error("Unknown funct in execute");
            }
            result.instructionsExecuted++;
            return;
        }

        // ── ADDI ────────────────────────────────────────────────────────────
        case O::ADDI:
            regs_.write(instr.rt, regs_.read(instr.rs) + instr.imm);
            result.instructionsExecuted++;
            return;
        case O::ANDI:
            regs_.write(instr.rt, regs_.read(instr.rs) & instr.imm);
            result.instructionsExecuted++;
            return;
        case O::ORI:
            regs_.write(instr.rt, regs_.read(instr.rs) | instr.imm);
            result.instructionsExecuted++;
            return;

        // ── Memory ────────────────────────────────────────────────────────
        case O::LW: {
            uint32_t addr = static_cast<uint32_t>(regs_.read(instr.rs) + instr.imm);
            uint32_t val  = mem_load(addr, result);
            regs_.write(instr.rt, static_cast<int32_t>(val));
            result.instructionsExecuted++;
            return;
        }
        case O::SW: {
            uint32_t addr = static_cast<uint32_t>(regs_.read(instr.rs) + instr.imm);
            mem_store(addr, static_cast<uint32_t>(regs_.read(instr.rt)), result);
            result.instructionsExecuted++;
            return;
        }

        // ── Branches (PC-relative, word offset) ─────────────────────────────
        // Branch target = (PC after fetch) + imm * 4
        // PC was already incremented by 4 before execute_one is called.
        case O::BEQ: {
            bool taken = regs_.read(instr.rs) == regs_.read(instr.rt);
            if (taken) pc_ = static_cast<uint32_t>(
                static_cast<int32_t>(pc_) + instr.imm * 4);
            result.instructionsExecuted++;
            return;
        }
        case O::BNE: {
            bool taken = regs_.read(instr.rs) != regs_.read(instr.rt);
            if (taken) pc_ = static_cast<uint32_t>(
                static_cast<int32_t>(pc_) + instr.imm * 4);
            result.instructionsExecuted++;
            return;
        }
        case O::BLT: {
            bool taken = regs_.read(instr.rs) < regs_.read(instr.rt);
            if (taken) pc_ = static_cast<uint32_t>(
                static_cast<int32_t>(pc_) + instr.imm * 4);
            result.instructionsExecuted++;
            return;
        }
        case O::BGE: {
            bool taken = regs_.read(instr.rs) >= regs_.read(instr.rt);
            if (taken) pc_ = static_cast<uint32_t>(
                static_cast<int32_t>(pc_) + instr.imm * 4);
            result.instructionsExecuted++;
            return;
        }

        // ── Jumps (absolute word address × 4) ───────────────────────────────
        case O::J:
            pc_ = instr.addr * 4;
            result.instructionsExecuted++;
            return;
        case O::JAL:
            regs_.write(15, static_cast<int32_t>(pc_)); // $r15 = return addr (already past instr)
            pc_ = instr.addr * 4;
            result.instructionsExecuted++;
            return;

        default:
            throw std::runtime_error("Unknown opcode in execute");
    }
}
