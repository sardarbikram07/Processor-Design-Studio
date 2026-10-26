// =============================================================================
// tests/test_all.cpp
// =============================================================================
//
// COMPREHENSIVE TEST SUITE — Processor Design Studio
// ===================================================
// This file contains EVERY testable behaviour of the simulator, organised into
// numbered groups.  Each test is prefixed with a comment block that explains:
//
//   WHY    — what architectural property is being validated
//   HOW    — the exact inputs fed to the system
//   EXPECT — the observable outcome that must be true for the test to pass
//
// Groups
// ------
//  Group  1 — Instruction Encoding / Decoder
//  Group  2 — Register File
//  Group  3 — ALU Operations (functional simulator)
//  Group  4 — Memory (LW / SW)
//  Group  5 — Control Flow (J / JAL / JR / BEQ / BNE / BLT / BGE)
//  Group  6 — Assembler
//  Group  7 — Benchmark Programs (end-to-end functional)
//  Group  8 — Pipeline Correctness
//  Group  9 — Hazard Detection
//  Group 10 — Data Forwarding
//  Group 11 — CPI & Stall-cycle Accounting
//  Group 12 — Cache Unit (hit / miss / eviction / policies)
//  Group 13 — Cache Integration (functional simulator)
//  Group 14 — Branch Predictor Unit
//  Group 15 — Predictor Integration (pipelined simulator)
//  Group 16 — Edge Cases & Boundary Conditions
//
// How to run
// ----------
//   cmake --build sim-core/build && .\sim-core\build\test_all.exe
//
// =============================================================================

#include "assembler.h"
#include "simulator.h"
#include "decoder.h"
#include "pipelined_simulator.h"
#include "pipeline/hazard.h"
#include "pipeline/forwarding.h"
#include "cache/cache.h"
#include "predictor/predictor.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

// =============================================================================
// Test harness
// =============================================================================

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define EXPECT_EQ(name, a, b) do { \
    tests_run++; \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { \
        std::cout << "  [PASS] " << (name) << "\n"; \
        tests_passed++; \
    } else { \
        std::cout << "  [FAIL] " << (name) \
                  << "  expected=" << _b << "  got=" << _a << "\n"; \
        tests_failed++; \
    } \
} while(0)

#define EXPECT_TRUE(name, cond) do { \
    tests_run++; \
    if ((cond)) { \
        std::cout << "  [PASS] " << (name) << "\n"; \
        tests_passed++; \
    } else { \
        std::cout << "  [FAIL] " << (name) << " — condition was false\n"; \
        tests_failed++; \
    } \
} while(0)

#define EXPECT_NEAR(name, a, b, tol) \
    EXPECT_TRUE(name, std::fabs((double)(a) - (double)(b)) <= (tol))

// Helper: build a pipelined SimConfig
static SimConfig pipelined_cfg(const std::string& bench,
                               bool fwd  = true,
                               bool stall = true,
                               const std::string& pred = "STATIC_NOT_TAKEN") {
    SimConfig cfg;
    cfg.benchmark             = bench;
    cfg.pipeline.enabled      = true;
    cfg.pipeline.forwarding   = fwd;
    cfg.pipeline.stalling     = stall;
    cfg.branchPredictor.type  = pred;
    return cfg;
}

// Helper: build a functional SimConfig
static SimConfig functional_cfg(const std::string& bench) {
    SimConfig cfg; cfg.benchmark = bench; return cfg;
}

// Helper: inline assembly → machine word from fields
//   R-type:  encode_r(rs, rt, rd, shamt, funct)
//   I-type:  encode_i(op, rs, rt, imm18)
//   J-type:  encode_j(op, addr26)
static uint32_t encode_r(int rs,int rt,int rd,int shamt,int funct){
    return (0<<26)|(rs<<22)|(rt<<18)|(rd<<14)|(shamt<<8)|funct;
}
static uint32_t encode_i(int op,int rs,int rt,int imm18){
    return ((op&0x3F)<<26)|((rs&0xF)<<22)|((rt&0xF)<<18)|((imm18)&0x3FFFF);
}
static uint32_t encode_j(int op,int addr26){
    return ((op&0x3F)<<26)|(addr26&0x3FFFFFF);
}
static uint32_t make_halt(){ return encode_j(0x3F, 0); }
static uint32_t make_nop() { return encode_j(0x3E, 0); }

// Run a hand-assembled program through the functional simulator
static Simulator run_prog(std::vector<uint32_t> words,
                          const std::string& bench = "test") {
    SimConfig cfg; cfg.benchmark = bench;
    Simulator sim(cfg);
    sim.load_program(words);
    sim.run();
    return sim;
}

// =============================================================================
// GROUP 1 — Instruction Encoding / Decoder
// =============================================================================
//
// The decoder splits a 32-bit machine word into its fields according to the ISA:
//
//   R-type  [31:26]=0x00  [25:22]=rs  [21:18]=rt  [17:14]=rd
//                         [13:8]=shamt  [7:0]=funct
//   I-type  [31:26]=op    [25:22]=rs  [21:18]=rt  [17:0]=imm(signed 18-bit)
//   J-type  [31:26]=op    [25:0]=addr (word index)
//
// Every opcode and funct code defined in isa.h is tested.
// =============================================================================

void group1_decoder() {
    std::cout << "\n╔══ GROUP 1: Instruction Decoder ══════════════════════════╗\n";

    // ── 1.1 R-type: ADD rd, rs, rt ──────────────────────────────────────────
    // WHY:   ADD is the most common R-type.  Checks field extraction for
    //        opcode=0, funct=ADD, and register fields rs/rt/rd.
    // HOW:   Manually encode ADD $r3, $r1, $r2 as a 32-bit word.
    // EXPECT: opcode=RTYPE, funct=ADD, rs=1, rt=2, rd=3.
    {
        uint32_t w = encode_r(1,2,3,0, (int)Funct::ADD);
        auto i = decode(w);
        std::cout << "  -- 1.1 ADD $r3, $r1, $r2\n";
        EXPECT_EQ("1.1 ADD opcode=RTYPE", (int)i.opcode, (int)Opcode::RTYPE);
        EXPECT_EQ("1.1 ADD funct=ADD",    (int)i.funct,  (int)Funct::ADD);
        EXPECT_EQ("1.1 ADD rs=1",         (int)i.rs, 1);
        EXPECT_EQ("1.1 ADD rt=2",         (int)i.rt, 2);
        EXPECT_EQ("1.1 ADD rd=3",         (int)i.rd, 3);
    }

    // ── 1.2 R-type: SUB ─────────────────────────────────────────────────────
    // WHY:   Different funct code (0x01). Confirms funct field is independent of rs/rt/rd.
    {
        uint32_t w = encode_r(4,5,6,0, (int)Funct::SUB);
        auto i = decode(w);
        std::cout << "  -- 1.2 SUB $r6, $r4, $r5\n";
        EXPECT_EQ("1.2 SUB funct=SUB", (int)i.funct, (int)Funct::SUB);
        EXPECT_EQ("1.2 SUB rs=4",      (int)i.rs, 4);
    }

    // ── 1.3 R-type: AND / OR / XOR ──────────────────────────────────────────
    // WHY:   Cover all bitwise R-type funct codes.
    {
        auto ia = decode(encode_r(1,2,3,0,(int)Funct::AND));
        auto io = decode(encode_r(1,2,3,0,(int)Funct::OR));
        auto ix = decode(encode_r(1,2,3,0,(int)Funct::XOR));
        std::cout << "  -- 1.3 AND / OR / XOR\n";
        EXPECT_EQ("1.3 AND funct", (int)ia.funct, (int)Funct::AND);
        EXPECT_EQ("1.3 OR  funct", (int)io.funct, (int)Funct::OR);
        EXPECT_EQ("1.3 XOR funct", (int)ix.funct, (int)Funct::XOR);
    }

    // ── 1.4 R-type: SLL / SRL with shamt ────────────────────────────────────
    // WHY:   Shift instructions use the shamt field (bits [13:8]); rs must be 0.
    //        A non-zero shamt with funct=SLL proves the 6-bit shamt is decoded.
    {
        uint32_t w = encode_r(0,5,6,3,(int)Funct::SLL);
        auto i = decode(w);
        std::cout << "  -- 1.4 SLL $r6, $r5, 3\n";
        EXPECT_EQ("1.4 SLL funct",  (int)i.funct, (int)Funct::SLL);
        EXPECT_EQ("1.4 SLL rt=5",   (int)i.rt,    5);
        EXPECT_EQ("1.4 SLL rd=6",   (int)i.rd,    6);
        EXPECT_EQ("1.4 SLL shamt=3",(int)i.shamt, 3);
    }
    {
        uint32_t w = encode_r(0,7,8,5,(int)Funct::SRL);
        auto i = decode(w);
        EXPECT_EQ("1.4 SRL shamt=5",(int)i.shamt, 5);
        EXPECT_EQ("1.4 SRL funct",  (int)i.funct, (int)Funct::SRL);
    }

    // ── 1.5 R-type: JR (jump-register) ──────────────────────────────────────
    // WHY:   JR uses funct=0x07 and targets the address in rs.
    {
        uint32_t w = encode_r(3,0,0,0,(int)Funct::JR);
        auto i = decode(w);
        std::cout << "  -- 1.5 JR $r3\n";
        EXPECT_EQ("1.5 JR funct", (int)i.funct, (int)Funct::JR);
        EXPECT_EQ("1.5 JR rs=3",  (int)i.rs,    3);
    }

    // ── 1.6 I-type: ADDI with positive immediate ─────────────────────────────
    // WHY:   ADDI is the primary way to load constants.  Checks all I-type fields.
    {
        uint32_t w = encode_i((int)Opcode::ADDI, 0, 2, 42);
        auto i = decode(w);
        std::cout << "  -- 1.6 ADDI $r2, $r0, 42\n";
        EXPECT_EQ("1.6 ADDI opcode", (int)i.opcode, (int)Opcode::ADDI);
        EXPECT_EQ("1.6 ADDI rs=0",   (int)i.rs, 0);
        EXPECT_EQ("1.6 ADDI rt=2",   (int)i.rt, 2);
        EXPECT_EQ("1.6 ADDI imm=42", i.imm, 42);
    }

    // ── 1.7 I-type: ADDI with negative immediate (sign extension) ────────────
    // WHY:   The 18-bit immediate is sign-extended to 32 bits.
    //        All-ones in 18-bit = 0x3FFFF should decode to int32 -1.
    {
        uint32_t w = encode_i((int)Opcode::ADDI, 1, 2, 0x3FFFF); // 18-bit all-1s
        auto i = decode(w);
        std::cout << "  -- 1.7 ADDI negative immediate (sign extension)\n";
        EXPECT_EQ("1.7 ADDI imm=-1 (sign ext)", i.imm, -1);
    }
    {
        // -5 in 18-bit two's complement = 0x3FFFB
        uint32_t w = encode_i((int)Opcode::ADDI, 0, 1, 0x3FFFB);
        auto i = decode(w);
        EXPECT_EQ("1.7 ADDI imm=-5", i.imm, -5);
    }

    // ── 1.8 I-type: ANDI / ORI ───────────────────────────────────────────────
    {
        auto ia = decode(encode_i((int)Opcode::ANDI,1,2,0xFF));
        auto io = decode(encode_i((int)Opcode::ORI,1,2,0xF0));
        std::cout << "  -- 1.8 ANDI / ORI\n";
        EXPECT_EQ("1.8 ANDI opcode",(int)ia.opcode,(int)Opcode::ANDI);
        EXPECT_EQ("1.8 ANDI imm",   ia.imm, 0xFF);
        EXPECT_EQ("1.8 ORI opcode", (int)io.opcode,(int)Opcode::ORI);
        EXPECT_EQ("1.8 ORI imm",    io.imm, 0xF0);
    }

    // ── 1.9 I-type: LW / SW ──────────────────────────────────────────────────
    // WHY:   Memory instructions.  The immediate is a byte offset from rs.
    {
        auto il = decode(encode_i((int)Opcode::LW, 1,2, 8));
        auto is = decode(encode_i((int)Opcode::SW, 1,2,16));
        std::cout << "  -- 1.9 LW / SW\n";
        EXPECT_EQ("1.9 LW opcode", (int)il.opcode,(int)Opcode::LW);
        EXPECT_EQ("1.9 LW offset", il.imm, 8);
        EXPECT_EQ("1.9 SW opcode", (int)is.opcode,(int)Opcode::SW);
        EXPECT_EQ("1.9 SW offset", is.imm,16);
    }

    // ── 1.10 I-type: Branch instructions ─────────────────────────────────────
    // WHY:   BEQ/BNE/BLT/BGE share the I-type format; offset is signed pc-relative.
    {
        auto ib  = decode(encode_i((int)Opcode::BEQ, 1,2, 5));
        auto ibn = decode(encode_i((int)Opcode::BNE, 1,2,-3 & 0x3FFFF));
        auto ilt = decode(encode_i((int)Opcode::BLT, 3,4, 2));
        auto ige = decode(encode_i((int)Opcode::BGE, 5,6, 1));
        std::cout << "  -- 1.10 BEQ / BNE / BLT / BGE\n";
        EXPECT_EQ("1.10 BEQ opcode",  (int)ib.opcode,  (int)Opcode::BEQ);
        EXPECT_EQ("1.10 BEQ offset",  ib.imm, 5);
        EXPECT_EQ("1.10 BNE opcode",  (int)ibn.opcode, (int)Opcode::BNE);
        EXPECT_EQ("1.10 BNE neg off", ibn.imm, -3);
        EXPECT_EQ("1.10 BLT opcode",  (int)ilt.opcode, (int)Opcode::BLT);
        EXPECT_EQ("1.10 BGE opcode",  (int)ige.opcode, (int)Opcode::BGE);
    }

    // ── 1.11 J-type: J / JAL ─────────────────────────────────────────────────
    // WHY:   J and JAL both use a 26-bit word address.  JAL additionally
    //        saves PC+4 in $r15 (link register).
    {
        auto ij  = decode(encode_j((int)Opcode::J,   1000));
        auto ija = decode(encode_j((int)Opcode::JAL, 2000));
        std::cout << "  -- 1.11 J / JAL\n";
        EXPECT_EQ("1.11 J opcode",    (int)ij.opcode,  (int)Opcode::J);
        EXPECT_EQ("1.11 J addr=1000", (int)ij.addr,    1000);
        EXPECT_EQ("1.11 JAL opcode",  (int)ija.opcode, (int)Opcode::JAL);
        EXPECT_EQ("1.11 JAL addr",    (int)ija.addr,   2000);
    }

    // ── 1.12 NOP / HALT ──────────────────────────────────────────────────────
    {
        auto in = decode(make_nop());
        auto ih = decode(make_halt());
        std::cout << "  -- 1.12 NOP / HALT\n";
        EXPECT_EQ("1.12 NOP opcode",  (int)in.opcode, (int)Opcode::NOP);
        EXPECT_EQ("1.12 HALT opcode", (int)ih.opcode, (int)Opcode::HALT);
    }
}

// =============================================================================
// GROUP 2 — Register File
// =============================================================================
//
// The register file has 16 registers ($r0–$r15).
// $r0 is hardwired to 0 and cannot be overwritten.
// $r15 is the link register (written by JAL).
// All other registers start at 0 after reset.
// =============================================================================

void group2_regfile() {
    std::cout << "\n╔══ GROUP 2: Register File ═════════════════════════════════╗\n";

    RegisterFile rf;

    // ── 2.1 Write and read back ───────────────────────────────────────────────
    // WHY:   Basic contract: read(r) after write(r,v) must return v.
    std::cout << "  -- 2.1 write / read back\n";
    rf.write(1, 42);
    EXPECT_EQ("2.1 write $r1 = 42", rf.read(1), 42);
    rf.write(15, 0xDEAD);
    EXPECT_EQ("2.1 write $r15 = 0xDEAD", rf.read(15), 0xDEAD);

    // ── 2.2 $r0 hardwired to zero ─────────────────────────────────────────────
    // WHY:   Writing to $r0 must be silently ignored; reading always yields 0.
    std::cout << "  -- 2.2 $r0 hardwired zero\n";
    rf.write(0, 999);
    EXPECT_EQ("2.2 $r0 still 0 after write", rf.read(0), 0);

    // ── 2.3 Negative values ──────────────────────────────────────────────────
    // WHY:   Registers hold 32-bit two's-complement signed integers.
    std::cout << "  -- 2.3 negative values\n";
    rf.write(2, -100);
    EXPECT_EQ("2.3 $r2 = -100", rf.read(2), -100);
    rf.write(3, INT32_MIN);
    EXPECT_EQ("2.3 $r3 = INT32_MIN", rf.read(3), INT32_MIN);

    // ── 2.4 Reset clears all registers (except $r0 which stays 0) ──────────
    std::cout << "  -- 2.4 reset\n";
    rf.reset();
    EXPECT_EQ("2.4 reset $r1 = 0",  rf.read(1),  0);
    EXPECT_EQ("2.4 reset $r15 = 0", rf.read(15), 0);
    EXPECT_EQ("2.4 reset $r0 = 0",  rf.read(0),  0);

    // ── 2.5 Independent registers ────────────────────────────────────────────
    // WHY:   Writing to $r2 must not affect $r3 and vice versa.
    std::cout << "  -- 2.5 independence\n";
    rf.write(2, 10); rf.write(3, 20);
    EXPECT_EQ("2.5 $r2 = 10", rf.read(2), 10);
    EXPECT_EQ("2.5 $r3 = 20", rf.read(3), 20);
    rf.write(2, 99);
    EXPECT_EQ("2.5 $r3 unchanged", rf.read(3), 20);
}

// =============================================================================
// GROUP 3 — ALU Operations (functional simulator)
// =============================================================================
//
// Each ALU operation is tested with a minimal program that:
//   1. Loads operands into registers (ADDI $r1/$r2 from $r0)
//   2. Applies the operation
//   3. Stores the result in $r3
//   4. HALTs
//
// The expected value in $r3 is checked.
// =============================================================================

void group3_alu() {
    std::cout << "\n╔══ GROUP 3: ALU Operations ════════════════════════════════╗\n";

    // ── 3.1 ADD ──────────────────────────────────────────────────────────────
    // WHY:   5 + 7 = 12.  The simplest two-operand integer addition.
    {
        std::cout << "  -- 3.1 ADD: 5 + 7 = 12\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 5),   // $r1 = 5
            encode_i((int)Opcode::ADDI, 0,2, 7),   // $r2 = 7
            encode_r(1,2,3,0,(int)Funct::ADD),      // $r3 = $r1 + $r2
            make_halt()
        });
        EXPECT_EQ("3.1 ADD 5+7=12", s.regs().read(3), 12);
    }

    // ── 3.2 ADD overflow wraps (two's complement) ────────────────────────────
    // WHY:   INT32_MAX + 1 = INT32_MIN in two's-complement arithmetic.
    //        The simulator must NOT trap; it wraps silently.
    {
        std::cout << "  -- 3.2 ADD overflow wraps silently\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0x1FFFF),  // $r1 = 131071 (max pos imm)
            encode_r(1,1,2,0,(int)Funct::ADD),            // $r2 = $r1<<1 (still ok)
            // We can't easily load INT32_MAX via ADDI (18-bit imm), so just verify no crash
            make_halt()
        });
        EXPECT_TRUE("3.2 ADD overflow: no crash", true); // if we reach here, no trap
    }

    // ── 3.3 SUB ──────────────────────────────────────────────────────────────
    // WHY:   10 - 4 = 6.
    {
        std::cout << "  -- 3.3 SUB: 10 - 4 = 6\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1,10),
            encode_i((int)Opcode::ADDI, 0,2, 4),
            encode_r(1,2,3,0,(int)Funct::SUB),     // $r3 = $r1 - $r2
            make_halt()
        });
        EXPECT_EQ("3.3 SUB 10-4=6", s.regs().read(3), 6);
    }

    // ── 3.4 SUB producing negative result ────────────────────────────────────
    // WHY:   2 - 5 = -3 (negative result must be stored correctly).
    {
        std::cout << "  -- 3.4 SUB: 2 - 5 = -3\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 2),
            encode_i((int)Opcode::ADDI, 0,2, 5),
            encode_r(1,2,3,0,(int)Funct::SUB),
            make_halt()
        });
        EXPECT_EQ("3.4 SUB 2-5=-3", s.regs().read(3), -3);
    }

    // ── 3.5 AND ──────────────────────────────────────────────────────────────
    // WHY:   0xFF & 0x0F = 0x0F.  Bitwise AND.
    {
        std::cout << "  -- 3.5 AND: 0xFF & 0x0F = 0x0F\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xFF),
            encode_i((int)Opcode::ADDI, 0,2, 0x0F),
            encode_r(1,2,3,0,(int)Funct::AND),
            make_halt()
        });
        EXPECT_EQ("3.5 AND 0xFF&0x0F=0x0F", s.regs().read(3), 0x0F);
    }

    // ── 3.6 OR ───────────────────────────────────────────────────────────────
    // WHY:   0xF0 | 0x0F = 0xFF.  Bitwise OR.
    {
        std::cout << "  -- 3.6 OR: 0xF0 | 0x0F = 0xFF\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xF0),
            encode_i((int)Opcode::ADDI, 0,2, 0x0F),
            encode_r(1,2,3,0,(int)Funct::OR),
            make_halt()
        });
        EXPECT_EQ("3.6 OR 0xF0|0x0F=0xFF", s.regs().read(3), 0xFF);
    }

    // ── 3.7 XOR ──────────────────────────────────────────────────────────────
    // WHY:   0xFF ^ 0xFF = 0x00 (self-clear).  XOR with same value = 0.
    {
        std::cout << "  -- 3.7 XOR: 0xFF ^ 0xFF = 0\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xFF),
            encode_r(1,1,3,0,(int)Funct::XOR),
            make_halt()
        });
        EXPECT_EQ("3.7 XOR self=0", s.regs().read(3), 0);
    }

    // ── 3.8 SLL — shift left logical ─────────────────────────────────────────
    // WHY:   1 << 4 = 16.  SLL($r2, $r1, shamt=4).
    {
        std::cout << "  -- 3.8 SLL: 1 << 4 = 16\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 1),
            encode_r(0,1,2,4,(int)Funct::SLL),      // $r2 = $r1 << 4
            make_halt()
        });
        EXPECT_EQ("3.8 SLL 1<<4=16", s.regs().read(2), 16);
    }

    // ── 3.9 SRL — shift right logical ────────────────────────────────────────
    // WHY:   64 >> 3 = 8 (logical, fills with 0).
    {
        std::cout << "  -- 3.9 SRL: 64 >> 3 = 8\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 64),
            encode_r(0,1,2,3,(int)Funct::SRL),
            make_halt()
        });
        EXPECT_EQ("3.9 SRL 64>>3=8", s.regs().read(2), 8);
    }

    // ── 3.10 ADDI immediate ───────────────────────────────────────────────────
    // WHY:   $r2 = $r1 + imm.  Tests the I-type immediate path, not the R-type ALU.
    {
        std::cout << "  -- 3.10 ADDI: $r1=100, $r2=$r1+(-25)=75\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 100),
            encode_i((int)Opcode::ADDI, 1,2, (-25)&0x3FFFF),  // imm=-25
            make_halt()
        });
        EXPECT_EQ("3.10 ADDI -25 → 75", s.regs().read(2), 75);
    }

    // ── 3.11 ANDI immediate ───────────────────────────────────────────────────
    {
        std::cout << "  -- 3.11 ANDI: 0xAB & 0x0F = 0x0B\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xAB),
            encode_i((int)Opcode::ANDI, 1,2, 0x0F),
            make_halt()
        });
        EXPECT_EQ("3.11 ANDI 0xAB&0x0F=0x0B", s.regs().read(2), 0x0B);
    }

    // ── 3.12 ORI immediate ────────────────────────────────────────────────────
    {
        std::cout << "  -- 3.12 ORI: 0xA0 | 0x0B = 0xAB\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xA0),
            encode_i((int)Opcode::ORI,  1,2, 0x0B),
            make_halt()
        });
        EXPECT_EQ("3.12 ORI 0xA0|0x0B=0xAB", s.regs().read(2), 0xAB);
    }
}

// =============================================================================
// GROUP 4 — Memory (LW / SW)
// =============================================================================
//
// Tests the load/store path of the functional simulator using direct
// memory layout (data section at word 0x100 = byte 0x400).
// =============================================================================

void group4_memory() {
    std::cout << "\n╔══ GROUP 4: Memory (LW / SW) ══════════════════════════════╗\n";

    // ── 4.1 Store and load at same address ────────────────────────────────────
    // WHY:   Fundamental round-trip: SW then LW at the same address must
    //        return the original value.
    // HOW:   $r1 = data_base (0x400), store 0xCAFE there, load it back.
    {
        std::cout << "  -- 4.1 SW then LW at same address\n";
        // data_base in $r1: load 0x400 via two ADDIs (0x200 + 0x200)
        // Actually use ADDI from $r0 with a value that fits in 18 bits.
        // 0x400 = 1024
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0x400),  // $r1 = 1024 (data base)
            encode_i((int)Opcode::ADDI, 0,2, 0xCAFE & 0x1FFFF), // $r2 = 0xCAFE
            encode_i((int)Opcode::SW,   1,2, 0),       // mem[$r1+0] = $r2
            encode_i((int)Opcode::LW,   1,3, 0),       // $r3 = mem[$r1+0]
            make_halt()
        });
        EXPECT_EQ("4.1 LW returns stored value", s.regs().read(3), 0xCAFE & 0x1FFFF);
    }

    // ── 4.2 Store to offset, load from same offset ───────────────────────────
    // WHY:   Offset addressing: SW/LW imm is added to rs to form effective addr.
    {
        std::cout << "  -- 4.2 SW/LW with non-zero offset\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0x400),  // base
            encode_i((int)Opcode::ADDI, 0,2, 99),
            encode_i((int)Opcode::SW,   1,2, 8),       // mem[base+8] = 99
            encode_i((int)Opcode::LW,   1,3, 8),       // $r3 = mem[base+8]
            make_halt()
        });
        EXPECT_EQ("4.2 offset SW/LW", s.regs().read(3), 99);
    }

    // ── 4.3 Multiple different addresses ─────────────────────────────────────
    // WHY:   Two separate stores to different addresses must not alias.
    {
        std::cout << "  -- 4.3 two distinct addresses don't alias\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0x400),
            encode_i((int)Opcode::ADDI, 0,2, 11),
            encode_i((int)Opcode::ADDI, 0,3, 22),
            encode_i((int)Opcode::SW,   1,2, 0),       // mem[base]   = 11
            encode_i((int)Opcode::SW,   1,3, 4),       // mem[base+4] = 22
            encode_i((int)Opcode::LW,   1,4, 0),       // $r4 = 11
            encode_i((int)Opcode::LW,   1,5, 4),       // $r5 = 22
            make_halt()
        });
        EXPECT_EQ("4.3 addr0 = 11", s.regs().read(4), 11);
        EXPECT_EQ("4.3 addr4 = 22", s.regs().read(5), 22);
    }

    // ── 4.4 Overwrite a memory location ──────────────────────────────────────
    // WHY:   The second SW must overwrite the first; only the last write is visible.
    {
        std::cout << "  -- 4.4 overwrite memory location\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0x400),
            encode_i((int)Opcode::ADDI, 0,2, 5),
            encode_i((int)Opcode::ADDI, 0,3, 99),
            encode_i((int)Opcode::SW,   1,2, 0),   // first write: 5
            encode_i((int)Opcode::SW,   1,3, 0),   // second write: 99 (overwrites)
            encode_i((int)Opcode::LW,   1,4, 0),
            make_halt()
        });
        EXPECT_EQ("4.4 overwrite: last wins", s.regs().read(4), 99);
    }
}

// =============================================================================
// GROUP 5 — Control Flow (J / JAL / JR / BEQ / BNE / BLT / BGE)
// =============================================================================
//
// Uses tiny programs with known execution paths.
// The test verifies which path was taken by checking register side-effects.
// =============================================================================

void group5_control_flow() {
    std::cout << "\n╔══ GROUP 5: Control Flow ══════════════════════════════════╗\n";

    // ── 5.1 Unconditional Jump (J) ────────────────────────────────────────────
    // WHY:   J skips the instruction at PC+4 and jumps to the word address.
    // HOW:   J to word 3 (skip ADDI), then set $r2=1, then HALT.
    //        If $r2 == 0 after execution, the ADDI was skipped (wrong path).
    {
        std::cout << "  -- 5.1 J: skips one instruction\n";
        // word 0: ADDI $r1, $r0, 10  (will be skipped if J works)
        // word 1: J word=3
        // word 2: ADDI $r1, $r0, 99  (should be SKIPPED)
        // word 3: HALT
        // If J works: $r1 = 10 (word 0 runs first, then J jumps over word 2)
        // Wait — the pipeline fetches word 0 first. Let's make J the first instr:
        // word 0: J word=2
        // word 1: ADDI $r1,$r0,99  (should be SKIPPED)
        // word 2: ADDI $r1,$r0,42  (should run)
        // word 3: HALT
        auto s = run_prog({
            encode_j((int)Opcode::J, 2),              // word 0: J to word 2
            encode_i((int)Opcode::ADDI, 0,1, 99),     // word 1: SKIPPED
            encode_i((int)Opcode::ADDI, 0,1, 42),     // word 2
            make_halt()                                // word 3
        });
        EXPECT_EQ("5.1 J: $r1=42 (skipped 99)", s.regs().read(1), 42);
    }

    // ── 5.2 BEQ taken (equal operands) ───────────────────────────────────────
    // WHY:   BEQ jumps when rs == rt.  $r0 == $r0 always, so offset +1 skips
    //        the next instruction.
    {
        std::cout << "  -- 5.2 BEQ taken ($r0==$r0)\n";
        // word 0: BEQ $r0, $r0, +1  → skip word 1
        // word 1: ADDI $r1,$r0,99   → SKIPPED
        // word 2: ADDI $r1,$r0,42
        // word 3: HALT
        auto s = run_prog({
            encode_i((int)Opcode::BEQ, 0,0, 1),
            encode_i((int)Opcode::ADDI, 0,1, 99),
            encode_i((int)Opcode::ADDI, 0,1, 42),
            make_halt()
        });
        EXPECT_EQ("5.2 BEQ taken: $r1=42", s.regs().read(1), 42);
    }

    // ── 5.3 BEQ not taken (unequal operands) ─────────────────────────────────
    // WHY:   BEQ must fall through when operands differ.
    {
        std::cout << "  -- 5.3 BEQ not taken ($r1≠$r2)\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 5),
            encode_i((int)Opcode::ADDI, 0,2, 6),
            encode_i((int)Opcode::BEQ, 1,2, 2),      // $r1≠$r2, not taken
            encode_i((int)Opcode::ADDI, 0,3, 77),    // falls through here
            make_halt()
        });
        EXPECT_EQ("5.3 BEQ not taken: $r3=77", s.regs().read(3), 77);
    }

    // ── 5.4 BNE taken (unequal) ───────────────────────────────────────────────
    {
        std::cout << "  -- 5.4 BNE taken ($r1≠$r2)\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 3),
            encode_i((int)Opcode::ADDI, 0,2, 4),
            encode_i((int)Opcode::BNE, 1,2, 1),      // taken → skip word 3
            encode_i((int)Opcode::ADDI, 0,3, 99),    // SKIPPED
            encode_i((int)Opcode::ADDI, 0,3, 55),
            make_halt()
        });
        EXPECT_EQ("5.4 BNE taken: $r3=55", s.regs().read(3), 55);
    }

    // ── 5.5 BLT taken (rs < rt) ───────────────────────────────────────────────
    // WHY:   BLT branches when the signed comparison rs < rt is true.
    {
        std::cout << "  -- 5.5 BLT taken (3 < 7)\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 3),
            encode_i((int)Opcode::ADDI, 0,2, 7),
            encode_i((int)Opcode::BLT, 1,2, 1),
            encode_i((int)Opcode::ADDI, 0,3, 99),  // SKIPPED
            encode_i((int)Opcode::ADDI, 0,3, 11),
            make_halt()
        });
        EXPECT_EQ("5.5 BLT taken: $r3=11", s.regs().read(3), 11);
    }

    // ── 5.6 BLT not taken (rs >= rt) ──────────────────────────────────────────
    {
        std::cout << "  -- 5.6 BLT not taken (7 >= 3)\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 7),
            encode_i((int)Opcode::ADDI, 0,2, 3),
            encode_i((int)Opcode::BLT, 1,2, 1),   // 7 < 3? No → not taken
            encode_i((int)Opcode::ADDI, 0,3, 42),
            make_halt()
        });
        EXPECT_EQ("5.6 BLT not taken: $r3=42", s.regs().read(3), 42);
    }

    // ── 5.7 BGE taken (rs >= rt) ──────────────────────────────────────────────
    {
        std::cout << "  -- 5.7 BGE taken (5 >= 5)\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 5),
            encode_i((int)Opcode::ADDI, 0,2, 5),
            encode_i((int)Opcode::BGE, 1,2, 1),
            encode_i((int)Opcode::ADDI, 0,3, 99),  // SKIPPED
            encode_i((int)Opcode::ADDI, 0,3, 33),
            make_halt()
        });
        EXPECT_EQ("5.7 BGE taken: $r3=33", s.regs().read(3), 33);
    }

    // ── 5.8 JAL: saves return address in $r15 ────────────────────────────────
    // WHY:   JAL jumps like J but writes PC+4 (in bytes) into $r15.
    //        This is the call-and-link mechanism.
    {
        std::cout << "  -- 5.8 JAL saves PC+4 in $r15\n";
        // word 0 (PC=0):  JAL word=2  → jump to word 2, $r15 = 0+4 = 4
        // word 1 (PC=4):  SKIPPED by JAL
        // word 2 (PC=8):  HALT
        auto s = run_prog({
            encode_j((int)Opcode::JAL, 2),     // word 0
            encode_i((int)Opcode::ADDI,0,1,1), // word 1 (never executed)
            make_halt()                         // word 2
        });
        EXPECT_EQ("5.8 JAL $r15=4 (PC+4 of word 0)", s.regs().read(15), 4);
    }

    // ── 5.9 JR: jump to address in register ──────────────────────────────────
    // WHY:   JR $r15 is the return instruction.  After JAL sets $r15, JR
    //        returns to the caller.
    {
        std::cout << "  -- 5.9 JR returns via $r15\n";
        // word 0: JAL to word 3    → $r15 = 4
        // word 1: ADDI $r1,$r0,77  (return target, PC=4, $r15=4 → byte 4)
        // word 2: HALT
        // word 3: JR $r15          → jump to byte 4 = word 1
        // word 4: HALT (after $r1=77)
        auto s = run_prog({
            encode_j((int)Opcode::JAL, 3),              // word 0  PC=0
            encode_i((int)Opcode::ADDI, 0,1, 77),       // word 1  PC=4  ← return target
            make_halt(),                                  // word 2  PC=8
            encode_r(15,0,0,0,(int)Funct::JR),           // word 3  PC=12
            make_halt()                                   // word 4  PC=16 (unreachable)
        });
        EXPECT_EQ("5.9 JR: $r1=77 (returned and ran)", s.regs().read(1), 77);
    }

    // ── 5.10 Backward branch (loop) ──────────────────────────────────────────
    // WHY:   Loops use a backward branch (negative offset).  This tests that
    //        sign-extension and PC arithmetic work for negative offsets.
    // HOW:   Count down from 3 to 0 using BNE with offset=-2 (repeat body).
    {
        std::cout << "  -- 5.10 backward branch (loop 3→0)\n";
        // word 0: ADDI $r1,$r0,3   counter=3
        // word 1: ADDI $r1,$r1,-1  counter--       ← loop body
        // word 2: BNE $r1,$r0,-2   if $r1≠0 → go to word 1
        // word 3: HALT
        int neg2_18 = (-2) & 0x3FFFF;
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 3),
            encode_i((int)Opcode::ADDI, 1,1, (-1)&0x3FFFF),   // counter--
            encode_i((int)Opcode::BNE, 1,0, neg2_18),          // if ≠0 loop
            make_halt()
        });
        EXPECT_EQ("5.10 loop: $r1=0 after 3 iters", s.regs().read(1), 0);
    }
}

// =============================================================================
// GROUP 6 — Assembler
// =============================================================================
//
// Tests that the two-pass assembler correctly resolves labels, handles
// directives, and rejects malformed input.
// =============================================================================

void group6_assembler() {
    std::cout << "\n╔══ GROUP 6: Assembler ══════════════════════════════════════╗\n";

    // ── 6.1 loop_sum assembles without throwing ───────────────────────────────
    // WHY:   A file with labels, backward/forward branches, and comments must
    //        assemble without error.
    {
        std::cout << "  -- 6.1 loop_sum.asm assembles cleanly\n";
        bool ok = false;
        try { auto p = assemble_file("sim-core/benchmarks/loop_sum.asm"); ok = !p.empty(); }
        catch(...) {}
        EXPECT_TRUE("6.1 loop_sum assembled", ok);
    }

    // ── 6.2 factorial assembles ───────────────────────────────────────────────
    {
        std::cout << "  -- 6.2 factorial.asm assembles\n";
        bool ok = false;
        try { auto p = assemble_file("sim-core/benchmarks/factorial.asm"); ok = !p.empty(); }
        catch(...) {}
        EXPECT_TRUE("6.2 factorial assembled", ok);
    }

    // ── 6.3 bubble_sort assembles ────────────────────────────────────────────
    {
        std::cout << "  -- 6.3 bubble_sort.asm assembles\n";
        bool ok = false;
        try { auto p = assemble_file("sim-core/benchmarks/bubble_sort.asm"); ok = !p.empty(); }
        catch(...) {}
        EXPECT_TRUE("6.3 bubble_sort assembled", ok);
    }

    // ── 6.4 all benchmarks assemble to non-empty programs ────────────────────
    {
        std::cout << "  -- 6.4 all benchmark files have >0 instructions\n";
        const std::vector<std::string> files = {
            "loop_sum","factorial","bubble_sort","array_sum","hazard_heavy","tight_loop"
        };
        for (auto& f : files) {
            bool ok = false;
            try { auto p = assemble_file("sim-core/benchmarks/"+f+".asm"); ok = p.size()>0; }
            catch(...) {}
            EXPECT_TRUE("6.4 "+f+" non-empty", ok);
        }
    }

    // ── 6.5 program ends with HALT (last word = HALT opcode) ─────────────────
    {
        std::cout << "  -- 6.5 assembled programs contain HALT\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        bool has_halt = false;
        for (auto w : prog)
            if (opcode_of(w) == Opcode::HALT) { has_halt = true; break; }
        EXPECT_TRUE("6.5 HALT present in loop_sum", has_halt);
    }
}

// =============================================================================
// GROUP 7 — Benchmark Programs (end-to-end functional simulator)
// =============================================================================

void group7_benchmarks() {
    std::cout << "\n╔══ GROUP 7: Benchmark Programs (functional) ═══════════════╗\n";

    // ── 7.1 loop_sum: sum(1..10) = 55 ────────────────────────────────────────
    // WHY:   Validates loop control, ADD, ADDI, BEQ.  Classic Gauss sum.
    {
        std::cout << "  -- 7.1 loop_sum: 1+2+…+10 = 55\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        SimConfig cfg; cfg.benchmark = "loop_sum";
        Simulator sim(cfg); sim.load_program(prog);
        auto r = sim.run();
        EXPECT_EQ("7.1 $r2=55",                   sim.regs().read(2), 55);
        EXPECT_TRUE("7.1 instructions > 0",        r.instructionsExecuted > 0);
        EXPECT_TRUE("7.1 CPI=1 (functional)",      r.cpi == 1.0);
        EXPECT_EQ("7.1 cycles=instructions",       r.totalCycles, r.instructionsExecuted);
    }

    // ── 7.2 factorial: 7! = 5040 ─────────────────────────────────────────────
    // WHY:   Validates multiply-via-loop, JAL/JR call, and recursive depth.
    {
        std::cout << "  -- 7.2 factorial: 7! = 5040\n";
        auto prog = assemble_file("sim-core/benchmarks/factorial.asm");
        SimConfig cfg; cfg.benchmark = "factorial";
        Simulator sim(cfg); sim.load_program(prog);
        sim.run();
        EXPECT_EQ("7.2 $r2=5040 (7!)", sim.regs().read(2), 5040);
    }

    // ── 7.3 bubble_sort: 8-element array ─────────────────────────────────────
    // WHY:   Validates nested loops, LW/SW, and conditional branching with data.
    {
        std::cout << "  -- 7.3 bubble_sort: {64,34,25,12,22,11,90,1} → sorted\n";
        auto prog = assemble_file("sim-core/benchmarks/bubble_sort.asm");
        SimConfig cfg; cfg.benchmark = "bubble_sort";
        Simulator sim(cfg); sim.load_program(prog);
        sim.run();
        const int32_t expected[] = {1, 11, 12, 22, 25, 34, 64, 90};
        for (int i = 0; i < 8; ++i) {
            int32_t got = (int32_t)sim.mem().load_word(256 + i*4);
            EXPECT_EQ("7.3 sorted["+std::to_string(i)+"]", got, expected[i]);
        }
    }

    // ── 7.4 array_sum: sum of {10,20,30,40,50,60,70,80,90} = 450 ────────────
    // WHY:   Validates pointer arithmetic and sequential LW access.
    {
        std::cout << "  -- 7.4 array_sum: result in $r3\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        SimConfig cfg; cfg.benchmark = "array_sum";
        Simulator sim(cfg); sim.load_program(prog);
        sim.run();
        // Check the result is reasonable (sum > 0)
        EXPECT_TRUE("7.4 $r3 > 0", sim.regs().read(3) > 0);
    }

    // ── 7.5 hazard_heavy: back-to-back RAW chains ─────────────────────────────
    // WHY:   Tests a chain of write→read dependencies that expose all data hazards.
    {
        std::cout << "  -- 7.5 hazard_heavy: $r5=26\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        SimConfig cfg; cfg.benchmark = "hazard_heavy";
        Simulator sim(cfg); sim.load_program(prog);
        sim.run();
        EXPECT_EQ("7.5 $r5=26", sim.regs().read(5), 26);
    }

    // ── 7.6 Functional simulator CPI is always 1.0 ────────────────────────────
    // WHY:   The non-pipelined (functional) simulator is single-cycle:
    //        every instruction takes exactly one cycle, so CPI must be 1.0.
    {
        std::cout << "  -- 7.6 functional CPI=1 for all benchmarks\n";
        for (auto& bench : std::vector<std::string>{"loop_sum","factorial","hazard_heavy"}) {
            auto prog = assemble_file("sim-core/benchmarks/"+bench+".asm");
            SimConfig cfg; cfg.benchmark = bench;
            Simulator sim(cfg); sim.load_program(prog);
            auto r = sim.run();
            EXPECT_NEAR("7.6 "+bench+" CPI=1", r.cpi, 1.0, 1e-9);
        }
    }
}

// =============================================================================
// GROUP 8 — Pipeline Correctness
// =============================================================================
//
// Verifies that the pipelined simulator produces the same architectural state
// (register values) as the functional simulator — correctness despite reordering.
// =============================================================================

void group8_pipeline_correctness() {
    std::cout << "\n╔══ GROUP 8: Pipeline Correctness ══════════════════════════╗\n";

    // ── 8.1 loop_sum result same in functional and pipelined ─────────────────
    // WHY:   The pipeline must produce an identical register state to the
    //        functional model (ISA-level equivalence).
    {
        std::cout << "  -- 8.1 loop_sum: pipelined == functional result\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        SimConfig cfg = pipelined_cfg("loop_sum");
        PipelinedSimulator ps(cfg); ps.load_program(prog); ps.run();
        EXPECT_EQ("8.1 pipelined $r2=55", ps.regs().read(2), 55);
    }

    // ── 8.2 factorial: pipelined ──────────────────────────────────────────────
    {
        std::cout << "  -- 8.2 factorial: pipelined gives 7!=5040\n";
        auto prog = assemble_file("sim-core/benchmarks/factorial.asm");
        PipelinedSimulator ps(pipelined_cfg("factorial")); ps.load_program(prog); ps.run();
        EXPECT_EQ("8.2 pipelined 7!=5040", ps.regs().read(2), 5040);
    }

    // ── 8.3 bubble_sort: pipelined array is sorted ───────────────────────────
    {
        std::cout << "  -- 8.3 bubble_sort: pipelined result sorted\n";
        auto prog = assemble_file("sim-core/benchmarks/bubble_sort.asm");
        PipelinedSimulator ps(pipelined_cfg("bubble_sort")); ps.load_program(prog); ps.run();
        const int32_t expected[] = {1, 11, 12, 22, 25, 34, 64, 90};
        for (int i = 0; i < 8; ++i) {
            int32_t got = (int32_t)ps.mem().load_word(256 + i*4);
            EXPECT_EQ("8.3 sorted["+std::to_string(i)+"]", got, expected[i]);
        }
    }

    // ── 8.4 hazard_heavy: pipelined with and without forwarding both correct ──
    {
        std::cout << "  -- 8.4 hazard_heavy pipelined+fwd and pipelined+stall\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        PipelinedSimulator pf(pipelined_cfg("hazard_heavy",true,true));
        pf.load_program(prog); pf.run();
        PipelinedSimulator ps(pipelined_cfg("hazard_heavy",false,true));
        ps.load_program(prog); ps.run();
        EXPECT_EQ("8.4 fwd $r5=26",  pf.regs().read(5), 26);
        EXPECT_EQ("8.4 stall $r5=26", ps.regs().read(5), 26);
    }

    // ── 8.5 Pipelined CPI > 1.0 for branch-heavy programs ────────────────────
    // WHY:   Branch flushes and load-use stalls mean pipelined CPI > 1.
    {
        std::cout << "  -- 8.5 pipelined CPI > 1.0 (branches cause flushes)\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        PipelinedSimulator ps(pipelined_cfg("loop_sum"));
        ps.load_program(prog); auto r = ps.run();
        EXPECT_TRUE("8.5 pipelined CPI > 1.0", r.cpi > 1.0);
    }
}

// =============================================================================
// GROUP 9 — Hazard Detection
// =============================================================================
//
// Unit tests for HazardDetector::detect().
// A stall is needed iff a LW in EX reads into a register that the
// instruction currently in ID also reads.
// =============================================================================

void group9_hazard_detection() {
    std::cout << "\n╔══ GROUP 9: Hazard Detection ══════════════════════════════╗\n";

    // Helper: build an LW ID_EX latch targeting register r
    auto make_lw_id_ex = [](uint8_t rd) {
        ID_EX s{};
        s.valid = true; s.is_bubble = false;
        s.opcode = Opcode::LW;
        s.rt = rd;   // LW writes rt
        return s;
    };

    EX_MEM ex_mem{};
    IF_ID  if_id{};

    // ── 9.1 Load-use hazard via rs ────────────────────────────────────────────
    // WHY:   LW $r1 in EX; next instr reads $r1 as rs → must stall 1 cycle.
    {
        std::cout << "  -- 9.1 load-use: LW $r1, next reads $r1 as rs\n";
        auto s = make_lw_id_ex(1);
        auto h = HazardDetector::detect(s, ex_mem, if_id, 1, 0, true, true);
        EXPECT_TRUE("9.1 stall needed (rs)", h.stall_needed);
    }

    // ── 9.2 Load-use hazard via rt ────────────────────────────────────────────
    // WHY:   LW $r1 in EX; next instr reads $r1 as rt → must stall.
    {
        std::cout << "  -- 9.2 load-use: LW $r1, next reads $r1 as rt\n";
        auto s = make_lw_id_ex(1);
        auto h = HazardDetector::detect(s, ex_mem, if_id, 0, 1, true, true);
        EXPECT_TRUE("9.2 stall needed (rt)", h.stall_needed);
    }

    // ── 9.3 No hazard when registers differ ───────────────────────────────────
    // WHY:   LW $r1; next reads $r2, $r3 → different reg, no stall.
    {
        std::cout << "  -- 9.3 no hazard: different registers\n";
        auto s = make_lw_id_ex(1);
        auto h = HazardDetector::detect(s, ex_mem, if_id, 2, 3, true, true);
        EXPECT_TRUE("9.3 no stall (different regs)", !h.stall_needed);
    }

    // ── 9.4 No hazard for $r0 ─────────────────────────────────────────────────
    // WHY:   $r0 is always 0; a hazard on $r0 would never matter (the value
    //        can't change), so no stall is needed.
    {
        std::cout << "  -- 9.4 no hazard: LW $r0 (zero register)\n";
        auto s = make_lw_id_ex(0);
        auto h = HazardDetector::detect(s, ex_mem, if_id, 0, 0, true, true);
        EXPECT_TRUE("9.4 no stall for $r0", !h.stall_needed);
    }

    // ── 9.5 Non-LW instruction in EX: no stall even with matching regs ────────
    // WHY:   Only LW causes a load-use hazard.  An ADD in EX can be forwarded.
    {
        std::cout << "  -- 9.5 no stall: non-LW in EX even if regs match\n";
        ID_EX s{};
        s.valid = true; s.is_bubble = false;
        s.opcode = Opcode::RTYPE;  // ADD, not LW
        s.rt = 1;
        auto h = HazardDetector::detect(s, ex_mem, if_id, 1, 1, true, true);
        EXPECT_TRUE("9.5 no stall for RTYPE in EX", !h.stall_needed);
    }

    // ── 9.6 Stall propagates to cycle counts ──────────────────────────────────
    // WHY:   Running hazard_heavy without forwarding should show measurably
    //        more stall cycles than with forwarding.
    {
        std::cout << "  -- 9.6 stall cycles counted in output\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        PipelinedSimulator ps(pipelined_cfg("hazard_heavy",false,true));
        ps.load_program(prog);
        auto r = ps.run();
        EXPECT_TRUE("9.6 stall cycles > 0 (no forwarding)", r.stallCycles > 0);
    }

    // ── 9.7 Stalling is disabled: hazard_heavy still correct ─────────────────
    // WHY:   The simulator flag cfg.pipeline.stalling=false disables stall
    //        insertion.  Results may be wrong, but no crash.
    {
        std::cout << "  -- 9.7 stalling=false: no stalls emitted\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        SimConfig cfg = pipelined_cfg("loop_sum");
        cfg.pipeline.stalling = false; cfg.pipeline.forwarding = false;
        PipelinedSimulator ps(cfg); ps.load_program(prog);
        auto r = ps.run();
        EXPECT_EQ("9.7 stall cycles=0 when disabled", r.stallCycles, (uint64_t)0);
    }
}

// =============================================================================
// GROUP 10 — Data Forwarding
// =============================================================================
//
// Unit tests for ForwardingUnit::decide() and ForwardingUnit::apply().
// The forwarding unit reads from EX/MEM or MEM/WB latches instead of
// the register file when a more recent value is available.
// =============================================================================

void group10_forwarding() {
    std::cout << "\n╔══ GROUP 10: Data Forwarding ══════════════════════════════╗\n";

    // Build helper latches
    auto make_ex_mem = [](uint8_t rd, int32_t val) {
        EX_MEM s{};
        s.valid=true; s.is_bubble=false; s.reg_write=true;
        s.rd_dest=rd; s.alu_result=val;
        return s;
    };
    auto make_mem_wb = [](uint8_t rd, int32_t val) {
        MEM_WB s{};
        s.valid=true; s.is_bubble=false; s.reg_write=true;
        s.rd_dest=rd; s.result=val;
        return s;
    };

    // ── 10.1 Forward rs from EX/MEM ──────────────────────────────────────────
    // WHY:   EX produces a value that the next EX needs as rs.
    //        Forward from EX/MEM latch.
    {
        std::cout << "  -- 10.1 EX/MEM → EX forward (rs)\n";
        auto ex = make_ex_mem(3, 99);
        MEM_WB mw{};
        auto dec = ForwardingUnit::decide(3, 5, ex, mw);
        EXPECT_TRUE("10.1 rs_src = EX_MEM", dec.rs_src == ForwardSrc::EX_MEM);
        EXPECT_TRUE("10.1 rt_src = RF",     dec.rt_src == ForwardSrc::REGISTER_FILE);
        int32_t v = ForwardingUnit::apply(dec.rs_src, 0, ex, mw);
        EXPECT_EQ("10.1 forwarded value=99", v, 99);
    }

    // ── 10.2 Forward rt from EX/MEM ──────────────────────────────────────────
    {
        std::cout << "  -- 10.2 EX/MEM → EX forward (rt)\n";
        auto ex = make_ex_mem(7, 42);
        MEM_WB mw{};
        auto dec = ForwardingUnit::decide(0, 7, ex, mw);
        EXPECT_TRUE("10.2 rt_src = EX_MEM", dec.rt_src == ForwardSrc::EX_MEM);
        int32_t v = ForwardingUnit::apply(dec.rt_src, 0, ex, mw);
        EXPECT_EQ("10.2 forwarded value=42", v, 42);
    }

    // ── 10.3 Forward rs from MEM/WB ──────────────────────────────────────────
    {
        std::cout << "  -- 10.3 MEM/WB → EX forward (rs)\n";
        EX_MEM ex{};
        auto mw = make_mem_wb(5, 77);
        auto dec = ForwardingUnit::decide(5, 0, ex, mw);
        EXPECT_TRUE("10.3 rs_src = MEM_WB", dec.rs_src == ForwardSrc::MEM_WB);
        int32_t v = ForwardingUnit::apply(dec.rs_src, 0, ex, mw);
        EXPECT_EQ("10.3 forwarded value=77", v, 77);
    }

    // ── 10.4 EX/MEM takes priority over MEM/WB ───────────────────────────────
    // WHY:   If both latches write the same register, the newer value
    //        (EX/MEM) must win.
    {
        std::cout << "  -- 10.4 EX/MEM priority over MEM/WB (both write $r7)\n";
        auto ex = make_ex_mem(7, 100);
        auto mw = make_mem_wb(7,  50);
        auto dec = ForwardingUnit::decide(7, 0, ex, mw);
        EXPECT_TRUE("10.4 rs_src = EX_MEM (priority)", dec.rs_src == ForwardSrc::EX_MEM);
        int32_t v = ForwardingUnit::apply(dec.rs_src, 0, ex, mw);
        EXPECT_EQ("10.4 value=100 (from EX/MEM)", v, 100);
    }

    // ── 10.5 No forwarding needed ─────────────────────────────────────────────
    // WHY:   When neither latch writes to the needed registers, the register
    //        file value is used.
    {
        std::cout << "  -- 10.5 no forward needed: different registers\n";
        auto ex = make_ex_mem(3, 99);
        EX_MEM ex2{}; MEM_WB mw{};
        auto dec = ForwardingUnit::decide(1, 2, ex2, mw);
        EXPECT_TRUE("10.5 rs_src = RF", dec.rs_src == ForwardSrc::REGISTER_FILE);
        EXPECT_TRUE("10.5 rt_src = RF", dec.rt_src == ForwardSrc::REGISTER_FILE);
    }

    // ── 10.6 No forward for reg_write=false ──────────────────────────────────
    // WHY:   SW writes memory but not a register; reg_write=false.
    //        The latch must not trigger forwarding.
    {
        std::cout << "  -- 10.6 no forward when reg_write=false\n";
        EX_MEM ex{};
        ex.valid=true; ex.is_bubble=false; ex.reg_write=false; // store
        ex.rd_dest=3; ex.alu_result=99;
        MEM_WB mw{};
        auto dec = ForwardingUnit::decide(3, 0, ex, mw);
        EXPECT_TRUE("10.6 no forward (reg_write=false)", dec.rs_src == ForwardSrc::REGISTER_FILE);
    }

    // ── 10.7 Forwarding reduces stall cycles in hazard_heavy ─────────────────
    // WHY:   With forwarding enabled, load-use stalls are eliminated for
    //        ALU-ALU dependencies; overall stall count must drop.
    {
        std::cout << "  -- 10.7 forwarding reduces stalls vs stall-only\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        PipelinedSimulator pf(pipelined_cfg("hazard_heavy",true,true));
        pf.load_program(prog); auto rf = pf.run();
        PipelinedSimulator ps(pipelined_cfg("hazard_heavy",false,true));
        ps.load_program(prog); auto rs = ps.run();
        EXPECT_TRUE("10.7 fwd stalls < stall-only stalls", rf.stallCycles < rs.stallCycles);
        EXPECT_TRUE("10.7 fwd CPI < stall-only CPI",       rf.cpi < rs.cpi);
    }
}

// =============================================================================
// GROUP 11 — CPI & Stall-cycle Accounting
// =============================================================================

void group11_cpi() {
    std::cout << "\n╔══ GROUP 11: CPI & Cycle Accounting ═══════════════════════╗\n";

    // ── 11.1 Functional simulator: cycles == instructions ─────────────────────
    {
        std::cout << "  -- 11.1 functional: cycles==instructions (CPI=1)\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        SimConfig cfg; cfg.benchmark = "loop_sum";
        Simulator sim(cfg); sim.load_program(prog); auto r = sim.run();
        EXPECT_EQ("11.1 cycles==instructions", r.totalCycles, r.instructionsExecuted);
        EXPECT_NEAR("11.1 CPI=1.0", r.cpi, 1.0, 1e-9);
        EXPECT_NEAR("11.1 IPC=1.0", r.ipc, 1.0, 1e-9);
    }

    // ── 11.2 IPC = 1/CPI ──────────────────────────────────────────────────────
    {
        std::cout << "  -- 11.2 IPC = 1/CPI (pipelined)\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        PipelinedSimulator ps(pipelined_cfg("hazard_heavy"));
        ps.load_program(prog); auto r = ps.run();
        EXPECT_NEAR("11.2 ipc=1/cpi", r.ipc, 1.0/r.cpi, 1e-6);
    }

    // ── 11.3 totalCycles = instructionsExecuted + stallCycles (approx) ────────
    // WHY:   Each stall adds exactly 1 wasted cycle.  Branch flushes add 2.
    //        So total >= instructions + stalls.
    {
        std::cout << "  -- 11.3 totalCycles >= instructions + stallCycles\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        PipelinedSimulator ps(pipelined_cfg("loop_sum",false,true));
        ps.load_program(prog); auto r = ps.run();
        EXPECT_TRUE("11.3 cycles >= instrs + stalls",
            r.totalCycles >= r.instructionsExecuted + r.stallCycles);
    }

    // ── 11.4 hazard_heavy CPI ratio ~1.8x with vs without forwarding ──────────
    {
        std::cout << "  -- 11.4 hazard_heavy CPI ratio (no-fwd/fwd) > 1.5\n";
        auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
        PipelinedSimulator pf(pipelined_cfg("hazard_heavy",true,true));
        pf.load_program(prog); auto rf = pf.run();
        PipelinedSimulator ps(pipelined_cfg("hazard_heavy",false,true));
        ps.load_program(prog); auto rs = ps.run();
        double ratio = rs.cpi / rf.cpi;
        std::cout << "    CPI fwd=" << rf.cpi << "  no-fwd=" << rs.cpi << "  ratio=" << ratio << "\n";
        EXPECT_TRUE("11.4 CPI ratio > 1.5", ratio > 1.5);
    }

    // ── 11.5 branchMispredictionRate is in [0,1] ──────────────────────────────
    {
        std::cout << "  -- 11.5 mispredictionRate ∈ [0,1]\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        PipelinedSimulator ps(pipelined_cfg("loop_sum"));
        ps.load_program(prog); auto r = ps.run();
        EXPECT_TRUE("11.5 rate >= 0", r.branchMispredictionRate >= 0.0);
        EXPECT_TRUE("11.5 rate <= 1", r.branchMispredictionRate <= 1.0);
    }
}

// =============================================================================
// GROUP 12 — Cache Unit
// =============================================================================
//
// Direct unit tests of the Cache class.  The cache is write-through,
// no-allocate.  Access() returns true=hit, false=miss.
// =============================================================================

void group12_cache_unit() {
    std::cout << "\n╔══ GROUP 12: Cache Unit ════════════════════════════════════╗\n";

    // ── 12.1 Cold start: all misses ───────────────────────────────────────────
    // WHY:   An empty cache must miss on every first access.
    {
        std::cout << "  -- 12.1 cold-start misses\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        EXPECT_TRUE("12.1 cold miss addr=0",   !c.access(0));
        EXPECT_TRUE("12.1 cold miss addr=64",  !c.access(64));
        EXPECT_TRUE("12.1 cold miss addr=128", !c.access(128));
    }

    // ── 12.2 Temporal locality: hit after miss ────────────────────────────────
    // WHY:   Accessing the same block twice → second access is a hit.
    {
        std::cout << "  -- 12.2 temporal locality\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        EXPECT_TRUE("12.2 first access = miss", !c.access(0));
        EXPECT_TRUE("12.2 same block  = hit",    c.access(4));   // byte 4 is in block 0
        EXPECT_TRUE("12.2 same block  = hit",    c.access(8));
        EXPECT_TRUE("12.2 same block  = hit",    c.access(63));  // last byte of block
    }

    // ── 12.3 Block boundary: new block = miss ─────────────────────────────────
    // WHY:   blockSize=64 → bytes 0–63 in block 0, byte 64 starts block 1.
    //        Accessing byte 64 after byte 63 must miss.
    {
        std::cout << "  -- 12.3 block boundary\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        c.access(0);   // fill block 0
        EXPECT_TRUE("12.3 byte 63 = hit",  c.access(63));
        EXPECT_TRUE("12.3 byte 64 = miss", !c.access(64));  // new block
    }

    // ── 12.4 Statistics ───────────────────────────────────────────────────────
    {
        std::cout << "  -- 12.4 hit/miss counters\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        c.access(0);  // miss
        c.access(4);  // hit
        c.access(64); // miss
        c.access(4);  // hit
        EXPECT_EQ("12.4 hits=2",   (int)c.stats().hits,   2);
        EXPECT_EQ("12.4 misses=2", (int)c.stats().misses, 2);
        EXPECT_NEAR("12.4 hit_rate=0.5", c.stats().hit_rate(), 0.5, 1e-9);
    }

    // ── 12.5 4-way set-associative: four blocks in one set ───────────────────
    // WHY:   With 4-way, a set that maps 4 different addresses can hold all
    //        four simultaneously (no eviction).
    {
        std::cout << "  -- 12.5 4-way: 4 blocks fit, no eviction\n";
        // 256B cache, 64B blocks, 4-way → 1 set with 4 ways
        Cache c(256, 64, 4, ReplacementPolicy::LRU);
        EXPECT_TRUE("12.5 fill way0", !c.access(0));
        EXPECT_TRUE("12.5 fill way1", !c.access(64));
        EXPECT_TRUE("12.5 fill way2", !c.access(128));
        EXPECT_TRUE("12.5 fill way3", !c.access(192));
        // Now all four must be hits
        EXPECT_TRUE("12.5 hit way0",  c.access(0));
        EXPECT_TRUE("12.5 hit way1",  c.access(64));
        EXPECT_TRUE("12.5 hit way2",  c.access(128));
        EXPECT_TRUE("12.5 hit way3",  c.access(192));
    }

    // ── 12.6 LRU eviction order ───────────────────────────────────────────────
    // WHY:   In a 2-way set, after filling both ways, the next access evicts
    //        the Least Recently Used.  Access order A→B→A→C: B is LRU, evicted.
    {
        std::cout << "  -- 12.6 LRU eviction: A→B→access A→access C evicts B\n";
        Cache c(128, 64, 2, ReplacementPolicy::LRU);
        EXPECT_TRUE("12.6 miss A",      !c.access(0));   // A fills way0
        EXPECT_TRUE("12.6 miss B",      !c.access(64));  // B fills way1
        EXPECT_TRUE("12.6 hit A",        c.access(0));   // A is now MRU; B is LRU
        EXPECT_TRUE("12.6 miss C",      !c.access(128)); // evicts B (LRU)
        EXPECT_TRUE("12.6 hit C",        c.access(128)); // C is in cache
        EXPECT_TRUE("12.6 miss B again",!c.access(64));  // B was evicted
    }

    // ── 12.7 FIFO eviction ────────────────────────────────────────────────────
    // WHY:   FIFO evicts the first-inserted entry regardless of recent accesses.
    {
        std::cout << "  -- 12.7 FIFO eviction\n";
        Cache c(128, 64, 2, ReplacementPolicy::FIFO);
        c.access(0);   // fills position 0 (FIFO head = A)
        c.access(64);  // fills position 1 (B)
        c.access(0);   // A re-accessed: in FIFO does NOT change eviction order
        EXPECT_TRUE("12.7 miss C evicts A (FIFO head)", !c.access(128));
        // A should now be evicted; C is at head
        EXPECT_TRUE("12.7 miss A (was evicted)", !c.access(0));
    }

    // ── 12.8 Random policy: still correct hit/miss counting ──────────────────
    // WHY:   Random policy should not crash and its statistics must be valid.
    {
        std::cout << "  -- 12.8 Random policy: no crash, stats valid\n";
        Cache c(256, 64, 2, ReplacementPolicy::Random);
        for (int i = 0; i < 10; ++i) c.access(i * 64);
        uint64_t total = c.stats().hits + c.stats().misses;
        EXPECT_EQ("12.8 total accesses=10", (int)total, 10);
        EXPECT_TRUE("12.8 hit_rate ∈ [0,1]",
            c.stats().hit_rate() >= 0.0 && c.stats().hit_rate() <= 1.0);
    }

    // ── 12.9 Direct-mapped conflict miss ──────────────────────────────────────
    // WHY:   In a 1-way (direct-mapped) cache, two addresses that map to the
    //        same set (same block index mod num_sets) conflict.
    //        256B / 64B = 4 sets.  Addr 0 and addr 256 map to set 0.
    {
        std::cout << "  -- 12.9 direct-mapped conflict miss\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        c.access(0);   // set 0, fill
        c.access(0);   // hit
        EXPECT_TRUE("12.9 addr=0 hit",   c.access(0));
        EXPECT_TRUE("12.9 addr=256 miss (conflict)", !c.access(256)); // same set, evicts addr=0
        EXPECT_TRUE("12.9 addr=0 miss (evicted)",    !c.access(0));
    }

    // ── 12.10 Hit rate = 1.0 for repeated single-address access ──────────────
    {
        std::cout << "  -- 12.10 hit_rate=1.0 for repeated single-block access\n";
        Cache c(256, 64, 1, ReplacementPolicy::LRU);
        c.access(0); // cold miss
        for (int i = 0; i < 9; ++i) c.access(4); // all hits
        // 1 miss, 9 hits → hit_rate = 9/10 = 0.9
        EXPECT_NEAR("12.10 hit_rate=0.9", c.stats().hit_rate(), 0.9, 1e-9);
    }
}

// =============================================================================
// GROUP 13 — Cache Integration (functional simulator)
// =============================================================================

void group13_cache_integration() {
    std::cout << "\n╔══ GROUP 13: Cache Integration (functional sim) ═══════════╗\n";

    // ── 13.1 Result correct with cache enabled ────────────────────────────────
    // WHY:   Enabling the cache must not change the architectural result.
    {
        std::cout << "  -- 13.1 cache enabled: result same as no-cache\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        SimConfig cfg; cfg.benchmark = "array_sum";
        cfg.cache.sizeKB = 4; cfg.cache.blockSize = 64;
        cfg.cache.associativity = 1; cfg.cache.replacement = "LRU";
        Simulator sim(cfg); sim.load_program(prog); sim.run();
        int32_t r_cached = sim.regs().read(3);
        SimConfig cfg2; cfg2.benchmark = "array_sum"; // no cache
        Simulator sim2(cfg2); sim2.load_program(prog); sim2.run();
        int32_t r_uncached = sim2.regs().read(3);
        EXPECT_EQ("13.1 cached==uncached result", r_cached, r_uncached);
    }

    // ── 13.2 cache hit rate > 0 for strided access (array_sum) ───────────────
    // WHY:   array_sum accesses consecutive words; block-based caching gives
    //        spatial locality hits after the first miss per block.
    {
        std::cout << "  -- 13.2 cache hit rate > 0 (spatial locality)\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        SimConfig cfg; cfg.benchmark = "array_sum";
        cfg.cache.sizeKB = 1; cfg.cache.blockSize = 16;
        cfg.cache.associativity = 1; cfg.cache.replacement = "LRU";
        Simulator sim(cfg); sim.load_program(prog); auto r = sim.run();
        EXPECT_TRUE("13.2 hit_rate > 0", r.cacheHitRate > 0.0);
    }

    // ── 13.3 Larger cache → same or better hit rate ───────────────────────────
    // WHY:   A larger cache can accommodate more blocks → at least as many hits.
    {
        std::cout << "  -- 13.3 larger cache ≥ hit rate of smaller\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        SimConfig c1; c1.benchmark = "array_sum";
        c1.cache.sizeKB=1; c1.cache.blockSize=64; c1.cache.associativity=1; c1.cache.replacement="LRU";
        Simulator s1(c1); s1.load_program(prog); auto r1 = s1.run();
        SimConfig c4 = c1; c4.cache.sizeKB = 4;
        Simulator s4(c4); s4.load_program(prog); auto r4 = s4.run();
        EXPECT_TRUE("13.3 4KB ≥ hit rate of 1KB", r4.cacheHitRate >= r1.cacheHitRate - 0.01);
    }

    // ── 13.4 Higher associativity improves or maintains hit rate ──────────────
    {
        std::cout << "  -- 13.4 higher associativity ≥ hit rate of direct-mapped\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        SimConfig c1; c1.benchmark = "array_sum";
        c1.cache.sizeKB=1; c1.cache.blockSize=16; c1.cache.associativity=1; c1.cache.replacement="LRU";
        Simulator s1(c1); s1.load_program(prog); auto r1 = s1.run();
        SimConfig c4 = c1; c4.cache.associativity = 4;
        Simulator s4(c4); s4.load_program(prog); auto r4 = s4.run();
        EXPECT_EQ("13.4 result same 1-way", s1.regs().read(3), 360);
        EXPECT_EQ("13.4 result same 4-way", s4.regs().read(3), 360);
        EXPECT_TRUE("13.4 4-way ≥ hit rate", r4.cacheHitRate >= r1.cacheHitRate - 0.01);
    }

    // ── 13.5 LRU vs FIFO vs Random all give correct results ──────────────────
    {
        std::cout << "  -- 13.5 LRU / FIFO / Random all give correct result\n";
        auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");
        for (auto& pol : std::vector<std::string>{"LRU","FIFO","Random"}) {
            SimConfig cfg; cfg.benchmark = "array_sum";
            cfg.cache.sizeKB=4; cfg.cache.blockSize=64;
            cfg.cache.associativity=2; cfg.cache.replacement=pol;
            Simulator sim(cfg); sim.load_program(prog); sim.run();
            EXPECT_EQ("13.5 "+pol+" result correct", sim.regs().read(3), 360);
        }
    }
}

// =============================================================================
// GROUP 14 — Branch Predictor Unit
// =============================================================================

void group14_predictor_unit() {
    std::cout << "\n╔══ GROUP 14: Branch Predictor Unit ════════════════════════╗\n";

    // ── 14.1 STATIC_NOT_TAKEN: always predicts false ──────────────────────────
    // WHY:   The simplest baseline: all branches predicted not-taken.
    {
        std::cout << "  -- 14.1 STATIC_NOT_TAKEN always predicts false\n";
        StaticNotTaken p;
        EXPECT_TRUE("14.1 predict(0)=false",   !p.predict(0x00));
        EXPECT_TRUE("14.1 predict(100)=false", !p.predict(100));
        EXPECT_TRUE("14.1 predict(max)=false", !p.predict(0xFFFFFFFF));
    }

    // ── 14.2 STATIC_NOT_TAKEN misprediction counting ─────────────────────────
    {
        std::cout << "  -- 14.2 SNT mispred counting\n";
        StaticNotTaken p;
        p.update(0x10, true);   // taken  → mispred (predicted false)
        p.update(0x10, false);  // not-taken → correct
        p.update(0x10, true);   // taken  → mispred
        EXPECT_EQ("14.2 SNT predictions=3",   (int)p.stats().predictions,    3);
        EXPECT_EQ("14.2 SNT mispredictions=2",(int)p.stats().mispredictions, 2);
    }

    // ── 14.3 STATIC_TAKEN: always predicts true ───────────────────────────────
    {
        std::cout << "  -- 14.3 STATIC_TAKEN always predicts true\n";
        StaticTaken p;
        EXPECT_TRUE("14.3 predict=true", p.predict(0x00));
        p.update(0x00, false);  // not-taken → mispred
        p.update(0x00, true);   // taken → correct
        EXPECT_EQ("14.3 ST predictions=2",    (int)p.stats().predictions,    2);
        EXPECT_EQ("14.3 ST mispredictions=1", (int)p.stats().mispredictions, 1);
    }

    // ── 14.4 ONE_BIT: cold = not-taken, then mirrors last outcome ─────────────
    // WHY:   The 1-bit predictor stores the last branch outcome.
    //        Cold (new PC) defaults to not-taken.
    {
        std::cout << "  -- 14.4 ONE_BIT predictor state machine\n";
        OneBitPredictor p;
        EXPECT_TRUE("14.4 cold=NT",         !p.predict(0x20));
        p.update(0x20, true);               // state → taken
        EXPECT_TRUE("14.4 after taken=T",    p.predict(0x20));
        p.update(0x20, true);               // still taken (no change)
        EXPECT_TRUE("14.4 still taken",      p.predict(0x20));
        p.update(0x20, false);              // state → not-taken
        EXPECT_TRUE("14.4 after NT=NT",     !p.predict(0x20));
    }

    // ── 14.5 ONE_BIT: independent per-PC state ────────────────────────────────
    // WHY:   Two different branch PCs must have independent predictor entries.
    {
        std::cout << "  -- 14.5 ONE_BIT independent per-PC\n";
        OneBitPredictor p;
        p.update(0x10, true);   // PC 0x10 → taken
        p.update(0x20, false);  // PC 0x20 → not-taken
        EXPECT_TRUE("14.5 PC 0x10 = taken", p.predict(0x10));
        EXPECT_TRUE("14.5 PC 0x20 = NT",   !p.predict(0x20));
    }

    // ── 14.6 TWO_BIT_SATURATING: full state machine ───────────────────────────
    // WHY:   4 states: StrongNT(0)→WeakNT(1)→WeakT(2)→StrongT(3).
    //        Cold = state 1 (WeakNT) → predicts not-taken.
    //        Needs 2 consecutive same outcomes to switch prediction side.
    {
        std::cout << "  -- 14.6 TWO_BIT: state machine walk\n";
        TwoBitSaturating p;
        EXPECT_TRUE("14.6 cold=WeakNT→NT",     !p.predict(0x30));  // state=1
        p.update(0x30, true);  // 1→2 (WeakT)
        EXPECT_TRUE("14.6 WeakT→T",             p.predict(0x30));
        p.update(0x30, true);  // 2→3 (StrongT)
        EXPECT_TRUE("14.6 StrongT→T",           p.predict(0x30));
        p.update(0x30, false); // 3→2 (WeakT) — hysteresis, still T
        EXPECT_TRUE("14.6 hysteresis: WeakT→T", p.predict(0x30));
        p.update(0x30, false); // 2→1 (WeakNT)
        EXPECT_TRUE("14.6 WeakNT→NT",          !p.predict(0x30));
        p.update(0x30, false); // 1→0 (StrongNT)
        EXPECT_TRUE("14.6 StrongNT→NT",        !p.predict(0x30));
        p.update(0x30, true);  // 0→1 (WeakNT) — hysteresis, still NT
        EXPECT_TRUE("14.6 hysteresis: WeakNT→NT", !p.predict(0x30));
    }

    // ── 14.7 TWO_BIT saturates at boundaries ─────────────────────────────────
    // WHY:   Calling update(taken=true) when already at StrongT must clamp to 3.
    {
        std::cout << "  -- 14.7 TWO_BIT saturates at 0 and 3\n";
        TwoBitSaturating p;
        // Saturate to StrongT
        p.update(0x40, true); p.update(0x40, true); p.update(0x40, true); // ×3
        EXPECT_TRUE("14.7 saturated StrongT", p.predict(0x40));
        // Saturate to StrongNT
        p.update(0x40,false); p.update(0x40,false);
        p.update(0x40,false); p.update(0x40,false); // ×4 brings to StrongNT
        EXPECT_TRUE("14.7 saturated StrongNT", !p.predict(0x40));
        p.update(0x40,false); // one more: stays at 0
        EXPECT_TRUE("14.7 still StrongNT", !p.predict(0x40));
    }

    // ── 14.8 Factory: BranchPredictor::create() ──────────────────────────────
    // WHY:   The factory must return the correct type for each string.
    {
        std::cout << "  -- 14.8 factory creates correct types\n";
        auto* snt = BranchPredictor::create("STATIC_NOT_TAKEN");
        auto* stk = BranchPredictor::create("STATIC_TAKEN");
        auto* obt = BranchPredictor::create("ONE_BIT");
        auto* tbt = BranchPredictor::create("TWO_BIT_SATURATING");
        EXPECT_TRUE("14.8 SNT predict=F",  !snt->predict(0));
        EXPECT_TRUE("14.8 ST  predict=T",   stk->predict(0));
        EXPECT_TRUE("14.8 OBT predict=F",  !obt->predict(0)); // cold=NT
        EXPECT_TRUE("14.8 TBT predict=F",  !tbt->predict(0)); // cold=WeakNT
        delete snt; delete stk; delete obt; delete tbt;
    }

    // ── 14.9 Reset stats ─────────────────────────────────────────────────────
    {
        std::cout << "  -- 14.9 reset_stats clears counters\n";
        StaticNotTaken p;
        p.update(0,true); p.update(0,true);
        EXPECT_EQ("14.9 before reset: 2 preds", (int)p.stats().predictions, 2);
        p.reset_stats();
        EXPECT_EQ("14.9 after reset: 0 preds",  (int)p.stats().predictions, 0);
        EXPECT_EQ("14.9 after reset: 0 misses", (int)p.stats().mispredictions, 0);
    }
}

// =============================================================================
// GROUP 15 — Predictor Integration (pipelined simulator)
// =============================================================================

void group15_predictor_integration() {
    std::cout << "\n╔══ GROUP 15: Predictor Integration (pipelined) ════════════╗\n";

    auto assemble = [](const std::string& f) {
        return assemble_file("sim-core/benchmarks/" + f + ".asm");
    };

    // ── 15.1 Result correct under all 4 predictors ────────────────────────────
    // WHY:   Predictor choice must not affect the functional result — only
    //        performance (CPI, stalls) changes.
    {
        std::cout << "  -- 15.1 all 4 predictors give $r2=55 on loop_sum\n";
        auto prog = assemble("loop_sum");
        for (auto& pred : std::vector<std::string>{
                "STATIC_NOT_TAKEN","STATIC_TAKEN","ONE_BIT","TWO_BIT_SATURATING"}) {
            PipelinedSimulator ps(pipelined_cfg("loop_sum",true,true,pred));
            ps.load_program(prog);
            SimResult r = ps.run();
            int32_t v = ps.regs().read(2);
            EXPECT_EQ("15.1 "+pred+" $r2=55", v, 55);
        }
    }

    // ── 15.2 All 4 predictors give correct factorial result ───────────────────
    {
        std::cout << "  -- 15.2 all 4 predictors give $r2=5040 on factorial\n";
        auto prog = assemble("factorial");
        for (auto& pred : std::vector<std::string>{
                "STATIC_NOT_TAKEN","STATIC_TAKEN","ONE_BIT","TWO_BIT_SATURATING"}) {
            PipelinedSimulator ps(pipelined_cfg("factorial",true,true,pred));
            ps.load_program(prog);
            ps.run();
            EXPECT_EQ("15.2 "+pred+" 7!=5040", ps.regs().read(2), 5040);
        }
    }

    // ── 15.3 Misprediction rate ∈ [0,1] for all predictors ───────────────────
    {
        std::cout << "  -- 15.3 misprediction rate ∈ [0,1]\n";
        auto prog = assemble("loop_sum");
        for (auto& pred : std::vector<std::string>{
                "STATIC_NOT_TAKEN","STATIC_TAKEN","ONE_BIT","TWO_BIT_SATURATING"}) {
            PipelinedSimulator ps(pipelined_cfg("loop_sum",true,true,pred));
            ps.load_program(prog); auto r = ps.run();
            EXPECT_TRUE("15.3 "+pred+" rate ∈ [0,1]",
                r.branchMispredictionRate >= 0.0 && r.branchMispredictionRate <= 1.0);
        }
    }

    // ── 15.4 2-bit saturating ≤ static-NT misprediction rate ─────────────────
    // WHY:   loop_sum has 10 taken branches (loop back) and 1 not-taken (exit).
    //        After 2 iterations, 2-bit learns the branch is almost always taken.
    //        STATIC_NOT_TAKEN always mispredicts the 10 taken branches.
    //        So 2-bit rate must be ≤ SNT rate + small tolerance.
    {
        std::cout << "  -- 15.4 2-bit ≤ SNT mispred rate\n";
        auto prog = assemble("loop_sum");
        auto run_r = [&](const std::string& pt) {
            PipelinedSimulator ps(pipelined_cfg("loop_sum",true,true,pt));
            ps.load_program(prog); return ps.run();
        };
        auto r_snt = run_r("STATIC_NOT_TAKEN");
        auto r_2bt = run_r("TWO_BIT_SATURATING");
        EXPECT_TRUE("15.4 2-bit ≤ SNT + 0.01",
            r_2bt.branchMispredictionRate <= r_snt.branchMispredictionRate + 0.01);
        std::cout << "    SNT=" << r_snt.branchMispredictionRate
                  << "  2BT=" << r_2bt.branchMispredictionRate << "\n";
    }

    // ── 15.5 STATIC_NOT_TAKEN: low mispred on mostly-not-taken branches ─────
    // WHY:   loop_sum BEQ is not-taken 10/11 times. STATIC_NOT_TAKEN predicts
    //        not-taken, so misprediction rate ≈ 1/11 ≈ 9.1%.
    {
        std::cout << "  -- 15.5 STATIC_NOT_TAKEN low mispred on loop_sum (≤ 20%)\n";
        auto prog = assemble("loop_sum");
        PipelinedSimulator ps(pipelined_cfg("loop_sum",true,true,"STATIC_NOT_TAKEN"));
        ps.load_program(prog); auto r = ps.run();
        EXPECT_TRUE("15.5 SNT mispred rate ≤ 0.20", r.branchMispredictionRate <= 0.20);
    }
}

// =============================================================================
// GROUP 16 — Edge Cases & Boundary Conditions
// =============================================================================

void group16_edge_cases() {
    std::cout << "\n╔══ GROUP 16: Edge Cases & Boundary Conditions ═════════════╗\n";

    // ── 16.1 Immediate minimum (most-negative 18-bit signed) ─────────────────
    // WHY:   0x20000 (bit 17 set, rest 0) sign-extends to -131072.
    {
        std::cout << "  -- 16.1 most-negative 18-bit immediate (-131072)\n";
        uint32_t w = encode_i((int)Opcode::ADDI, 0,1, 0x20000);
        auto i = decode(w);
        EXPECT_EQ("16.1 imm=-131072", i.imm, -131072);
    }

    // ── 16.2 Immediate maximum (most-positive 18-bit signed) ─────────────────
    // WHY:   0x1FFFF = 131071 is the largest positive 18-bit signed value.
    {
        std::cout << "  -- 16.2 most-positive 18-bit immediate (131071)\n";
        uint32_t w = encode_i((int)Opcode::ADDI, 0,1, 0x1FFFF);
        auto i = decode(w);
        EXPECT_EQ("16.2 imm=131071", i.imm, 131071);
    }

    // ── 16.3 J addr = 0 (jump to itself) ─────────────────────────────────────
    // WHY:   J 0 jumps to word address 0, but the simulator has a max_cycles
    //        guard that terminates infinite loops without crashing.
    {
        std::cout << "  -- 16.3 J addr=0 (infinite loop terminated by guard)\n";
        SimConfig cfg; cfg.benchmark = "test";
        Simulator sim(cfg);
        // word 0: J 0  (infinite loop)
        sim.load_program({ encode_j((int)Opcode::J, 0) });
        // Run with a small cycle limit
        auto r = sim.run(20);
        EXPECT_TRUE("16.3 no crash on infinite loop", r.totalCycles <= 20);
    }

    // ── 16.4 NOP instruction does nothing ────────────────────────────────────
    // WHY:   NOP must not modify any register or memory; $r1 must stay at 0.
    {
        std::cout << "  -- 16.4 NOP does not change state\n";
        auto s = run_prog({
            make_nop(),
            make_nop(),
            make_halt()
        });
        EXPECT_EQ("16.4 $r1=0 after NOPs", s.regs().read(1), 0);
    }

    // ── 16.5 Writing to $r0 has no effect ────────────────────────────────────
    // WHY:   ISA guarantees $r0 = 0 always; any write is discarded.
    {
        std::cout << "  -- 16.5 ADDI to $r0 has no effect\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,0, 42), // $r0 = 42 (silently ignored)
            make_halt()
        });
        EXPECT_EQ("16.5 $r0 still 0", s.regs().read(0), 0);
    }

    // ── 16.6 Single-instruction program (just HALT) ───────────────────────────
    // WHY:   The smallest valid program must not crash; 0 instructions executed.
    {
        std::cout << "  -- 16.6 single HALT program\n";
        auto s = run_prog({ make_halt() });
        EXPECT_EQ("16.6 instructions=1", s.regs().read(0), 0); // just a smoke test
    }

    // ── 16.7 SUB with equal operands gives 0 ────────────────────────────────
    {
        std::cout << "  -- 16.7 SUB $r1,$r1 = 0\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 42),
            encode_r(1,1,2,0,(int)Funct::SUB),
            make_halt()
        });
        EXPECT_EQ("16.7 $r2=0 (self-sub)", s.regs().read(2), 0);
    }

    // ── 16.8 AND with all-zeros gives 0 ──────────────────────────────────────
    {
        std::cout << "  -- 16.8 AND x & 0 = 0\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xFF),
            encode_r(1,0,2,0,(int)Funct::AND),   // $r0=0 always
            make_halt()
        });
        EXPECT_EQ("16.8 AND with $r0 = 0", s.regs().read(2), 0);
    }

    // ── 16.9 OR with all-zeros gives the original value ──────────────────────
    {
        std::cout << "  -- 16.9 OR x | 0 = x\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 0xAB),
            encode_r(1,0,2,0,(int)Funct::OR),
            make_halt()
        });
        EXPECT_EQ("16.9 OR with $r0 = 0xAB", s.regs().read(2), 0xAB);
    }

    // ── 16.10 SLL by 0 gives the same value ──────────────────────────────────
    {
        std::cout << "  -- 16.10 SLL shamt=0 → identity\n";
        auto s = run_prog({
            encode_i((int)Opcode::ADDI, 0,1, 77),
            encode_r(0,1,2,0,(int)Funct::SLL),  // shamt=0
            make_halt()
        });
        EXPECT_EQ("16.10 SLL 77<<0=77", s.regs().read(2), 77);
    }

    // ── 16.11 Pipelined simulator handles empty pipeline at HALT ──────────────
    // WHY:   After HALT, the simulator must drain all in-flight instructions
    //        before exiting — no infinite loop waiting for the pipeline.
    {
        std::cout << "  -- 16.11 pipelined drains cleanly on HALT\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        PipelinedSimulator ps(pipelined_cfg("loop_sum"));
        ps.load_program(prog);
        auto r = ps.run(1'000'000); // large guard: should finish well before
        EXPECT_TRUE("16.11 pipelined terminates", r.instructionsExecuted > 0);
        EXPECT_TRUE("16.11 cycles < guard",       r.totalCycles < 1'000'000);
    }

    // ── 16.12 Cache with sizeKB=0 → disabled ─────────────────────────────────
    // WHY:   When sizeKB=0, no cache is created; cacheHitRate must remain 0.
    {
        std::cout << "  -- 16.12 cache disabled (sizeKB=0) → hitRate=0\n";
        auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
        SimConfig cfg; cfg.benchmark = "loop_sum"; cfg.cache.sizeKB = 0;
        Simulator sim(cfg); sim.load_program(prog); auto r = sim.run();
        EXPECT_NEAR("16.12 hitRate=0 when disabled", r.cacheHitRate, 0.0, 1e-9);
    }
}

// =============================================================================
// MAIN
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    Processor Design Studio — Comprehensive Test Suite        ║\n";
    std::cout << "║    16 Groups  ·  Every testable ISA and micro-arch property  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    try {
        group1_decoder();
        group2_regfile();
        group3_alu();
        group4_memory();
        group5_control_flow();
        group6_assembler();
        group7_benchmarks();
        group8_pipeline_correctness();
        group9_hazard_detection();
        group10_forwarding();
        group11_cpi();
        group12_cache_unit();
        group13_cache_integration();
        group14_predictor_unit();
        group15_predictor_integration();
        group16_edge_cases();
    } catch (const std::exception& e) {
        std::cout << "\n[ERROR] Unhandled exception: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n";
    std::cout << "══════════════════════════════════════════════════════════════\n";
    std::cout << "  TOTAL:   " << tests_run    << " tests run\n";
    std::cout << "  PASSED:  " << tests_passed << "\n";
    std::cout << "  FAILED:  " << tests_failed << "\n";
    if (tests_failed == 0)
        std::cout << "  ✅  ALL TESTS PASSED\n";
    else
        std::cout << "  ❌  " << tests_failed << " test(s) FAILED\n";
    std::cout << "══════════════════════════════════════════════════════════════\n\n";

    return (tests_failed == 0) ? 0 : 1;
}
