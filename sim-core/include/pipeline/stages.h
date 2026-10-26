#pragma once
#include "isa.h"
#include <cstdint>

// ─── Pipeline Register structs ────────────────────────────────────────────────
// Each struct is the register "latch" between two stages.

// IF/ID latch
struct IF_ID {
    uint32_t pc          = 0;   // PC of the fetched instruction (byte addr)
    uint32_t instruction = 0;   // Raw 32-bit instruction word
    bool     valid       = false;
};

// ID/EX latch
struct ID_EX {
    uint32_t pc          = 0;
    Opcode   opcode      = Opcode::NOP;
    Funct    funct       = Funct::ADD;
    InstrFormat format   = InstrFormat::SPECIAL;
    uint8_t  rs          = 0;
    uint8_t  rt          = 0;
    uint8_t  rd          = 0;
    uint8_t  shamt       = 0;
    int32_t  imm         = 0;
    uint32_t addr        = 0;
    int32_t  rs_val      = 0;   // read from register file
    int32_t  rt_val      = 0;   // read from register file
    bool     valid       = false;
    bool     is_bubble   = false; // NOP inserted by stall/flush
};

// EX/MEM latch
struct EX_MEM {
    uint32_t pc          = 0;
    Opcode   opcode      = Opcode::NOP;
    uint8_t  rt          = 0;
    uint8_t  rd_dest     = 0;   // destination register (after resolution)
    int32_t  alu_result  = 0;   // ALU output
    int32_t  rt_val      = 0;   // for SW (value to store)
    bool     mem_read    = false;
    bool     mem_write   = false;
    bool     reg_write   = false;
    bool     branch_taken = false;
    uint32_t branch_target = 0;
    bool     valid       = false;
    bool     is_bubble   = false;
};

// MEM/WB latch
struct MEM_WB {
    uint32_t pc          = 0;
    Opcode   opcode      = Opcode::NOP;
    uint8_t  rd_dest     = 0;
    int32_t  result      = 0;   // writeback value (ALU or mem load)
    bool     reg_write   = false;
    bool     valid       = false;
    bool     is_bubble   = false;
};

// ─── Pipeline state ───────────────────────────────────────────────────────────
struct PipelineState {
    IF_ID   if_id;
    ID_EX   id_ex;
    EX_MEM  ex_mem;
    MEM_WB  mem_wb;

    void flush_fetch() {
        // Called on branch misprediction — squash IF/ID and ID/EX
        if_id  = IF_ID{};
        id_ex.is_bubble = true;
        id_ex.valid     = false;
    }

    void stall_fetch() {
        // Insert bubble into ID/EX, keep IF/ID frozen (PC not advanced)
        // The calling code must also not advance PC
        id_ex.is_bubble = true;
        id_ex.valid     = false;
    }
};
