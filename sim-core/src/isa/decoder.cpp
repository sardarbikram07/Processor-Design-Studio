#include "decoder.h"
#include <stdexcept>
#include <string>

Instruction decode(uint32_t word) {
    Instruction instr{};
    instr.raw    = word;
    instr.opcode = static_cast<Opcode>((word >> 26) & 0x3F);

    switch (instr.opcode) {
        // ── R-type ─────────────────────────────────────────────────────────
        case Opcode::RTYPE: {
            instr.format = InstrFormat::R;
            instr.rs     = (word >> 22) & 0xF;
            instr.rt     = (word >> 18) & 0xF;
            instr.rd     = (word >> 14) & 0xF;
            instr.shamt  = (word >>  8) & 0x3F;
            instr.funct  = static_cast<Funct>(word & 0xFF);

            // Validate funct
            switch (instr.funct) {
                case Funct::ADD: case Funct::SUB: case Funct::AND:
                case Funct::OR:  case Funct::XOR: case Funct::SLL:
                case Funct::SRL: case Funct::JR:
                    break;
                default:
                    throw std::runtime_error(
                        "Unknown funct: 0x" + std::to_string(static_cast<int>(instr.funct)));
            }
            break;
        }

        // ── I-type ─────────────────────────────────────────────────────────
        case Opcode::ADDI: case Opcode::ANDI: case Opcode::ORI:
        case Opcode::LW:   case Opcode::SW:
        case Opcode::BEQ:  case Opcode::BNE:
        case Opcode::BLT:  case Opcode::BGE: {
            instr.format = InstrFormat::I;
            instr.rs     = (word >> 22) & 0xF;
            instr.rt     = (word >> 18) & 0xF;
            // Sign-extend 18-bit immediate
            uint32_t raw_imm = word & 0x3FFFF;
            if (raw_imm & 0x20000)              // bit 17 set → negative
                instr.imm = static_cast<int32_t>(raw_imm | 0xFFFC0000u);
            else
                instr.imm = static_cast<int32_t>(raw_imm);
            break;
        }

        // ── J-type ─────────────────────────────────────────────────────────
        case Opcode::J: case Opcode::JAL: {
            instr.format = InstrFormat::J;
            instr.addr   = word & 0x3FFFFFF;   // 26-bit word index
            break;
        }

        // ── Special ────────────────────────────────────────────────────────
        case Opcode::NOP: case Opcode::HALT: {
            instr.format = InstrFormat::SPECIAL;
            break;
        }

        default:
            throw std::runtime_error(
                "Unknown opcode: 0x" + std::to_string(static_cast<int>(instr.opcode)));
    }

    return instr;
}
