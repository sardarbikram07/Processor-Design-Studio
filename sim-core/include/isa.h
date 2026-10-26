#pragma once
#include <cstdint>

// ─── Instruction Formats ────────────────────────────────────────────────────
// R-type: | opcode(6) | rs(4) | rt(4) | rd(4) | shamt(6) | funct(8) |
// I-type: | opcode(6) | rs(4) | rt(4) | immediate(18, signed)         |
// J-type: | opcode(6) | address(26, word index)                        |

enum class InstrFormat { R, I, J, SPECIAL };

// ─── Opcodes ────────────────────────────────────────────────────────────────
enum class Opcode : uint8_t {
    RTYPE = 0x00,   // All R-type use opcode 0x00; funct differentiates
    ADDI  = 0x01,
    ANDI  = 0x02,
    ORI   = 0x03,
    LW    = 0x04,
    SW    = 0x05,
    BEQ   = 0x06,
    BNE   = 0x07,
    BLT   = 0x08,
    BGE   = 0x09,
    J     = 0x0A,
    JAL   = 0x0B,
    NOP   = 0x3E,
    HALT  = 0x3F,
};

// ─── Funct codes (R-type only) ───────────────────────────────────────────────
enum class Funct : uint8_t {
    ADD = 0x00,
    SUB = 0x01,
    AND = 0x02,
    OR  = 0x03,
    XOR = 0x04,
    SLL = 0x05,
    SRL = 0x06,
    JR  = 0x07,
};

// ─── Decoded instruction ─────────────────────────────────────────────────────
struct Instruction {
    uint32_t    raw;        // Original 32-bit word
    InstrFormat format;
    Opcode      opcode;
    Funct       funct;      // R-type only
    uint8_t     rs;         // Source reg 1  (4-bit → reg 0–15)
    uint8_t     rt;         // Source reg 2 / dest for I-type
    uint8_t     rd;         // Dest reg (R-type)
    uint8_t     shamt;      // Shift amount (6-bit)
    int32_t     imm;        // Sign-extended 18-bit immediate (I-type)
    uint32_t    addr;       // 26-bit word address (J-type)
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
inline Opcode opcode_of(uint32_t word) {
    return static_cast<Opcode>((word >> 26) & 0x3F);
}
