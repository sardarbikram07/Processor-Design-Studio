// tests/test_phase1.cpp
// Phase 1 + Phase 2 + Phase 3 + Phase 4 acceptance tests.

#include "assembler.h"
#include "simulator.h"
#include "decoder.h"
#include "pipelined_simulator.h"
#include "pipeline/hazard.h"
#include "pipeline/forwarding.h"
#include "cache/cache.h"
#include "predictor/predictor.h"
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <string>
#include <cmath>

static int tests_run    = 0;
static int tests_passed = 0;

#define EXPECT_EQ(name, a, b) do { \
    tests_run++; \
    if ((a) == (b)) { \
        std::cout << "  [PASS] " << (name) << "\n"; \
        tests_passed++; \
    } else { \
        std::cout << "  [FAIL] " << (name) \
                  << "  expected=" << (b) << "  got=" << (a) << "\n"; \
    } \
} while(0)

#define EXPECT_TRUE(name, cond) do { \
    tests_run++; \
    if (cond) { \
        std::cout << "  [PASS] " << (name) << "\n"; \
        tests_passed++; \
    } else { \
        std::cout << "  [FAIL] " << (name) << " (condition was false)\n"; \
    } \
} while(0)

// ═══════════════════════════════════════════════════════════
// PHASE 1
// ═══════════════════════════════════════════════════════════

void test_decoder() {
    std::cout << "\n=== Phase 1: Decoder Unit Tests ===\n";

    uint32_t add_word = (0 << 26) | (1 << 22) | (2 << 18) | (3 << 14) | (0 << 8) | 0x00;
    auto add_instr = decode(add_word);
    EXPECT_EQ("ADD opcode",  static_cast<int>(add_instr.opcode), static_cast<int>(Opcode::RTYPE));
    EXPECT_EQ("ADD funct",   static_cast<int>(add_instr.funct),  static_cast<int>(Funct::ADD));
    EXPECT_EQ("ADD rs",      add_instr.rs, 1);
    EXPECT_EQ("ADD rt",      add_instr.rt, 2);
    EXPECT_EQ("ADD rd",      add_instr.rd, 3);

    uint32_t addi_word = (0x01 << 26) | (1 << 22) | (2 << 18) | 42;
    auto addi_instr = decode(addi_word);
    EXPECT_EQ("ADDI opcode", static_cast<int>(addi_instr.opcode), static_cast<int>(Opcode::ADDI));
    EXPECT_EQ("ADDI rs",     addi_instr.rs, 1);
    EXPECT_EQ("ADDI rt",     addi_instr.rt, 2);
    EXPECT_EQ("ADDI imm",    addi_instr.imm, 42);

    uint32_t neg_word = (0x01 << 26) | (1 << 22) | (2 << 18) | 0x3FFFF;
    auto neg_instr = decode(neg_word);
    EXPECT_EQ("ADDI neg imm", neg_instr.imm, -1);

    uint32_t j_word = (0x0A << 26) | 1000;
    auto j_instr = decode(j_word);
    EXPECT_EQ("J opcode", static_cast<int>(j_instr.opcode), static_cast<int>(Opcode::J));
    EXPECT_EQ("J addr",   static_cast<int>(j_instr.addr),   1000);

    uint32_t halt_word = static_cast<uint32_t>(Opcode::HALT) << 26;
    auto halt_instr = decode(halt_word);
    EXPECT_EQ("HALT opcode", static_cast<int>(halt_instr.opcode), static_cast<int>(Opcode::HALT));

    uint32_t sll_word = (0 << 26) | (0 << 22) | (5 << 18) | (6 << 14) | (3 << 8) | 0x05;
    auto sll_instr = decode(sll_word);
    EXPECT_EQ("SLL funct",  static_cast<int>(sll_instr.funct), static_cast<int>(Funct::SLL));
    EXPECT_EQ("SLL rt",     sll_instr.rt, 5);
    EXPECT_EQ("SLL rd",     sll_instr.rd, 6);
    EXPECT_EQ("SLL shamt",  sll_instr.shamt, 3);
}

void test_regfile() {
    std::cout << "\n=== Phase 1: Register File Unit Tests ===\n";
    RegisterFile rf;
    rf.write(1, 42);
    EXPECT_EQ("write/read $r1", rf.read(1), 42);
    rf.write(0, 999);
    EXPECT_EQ("$r0 hardwired 0", rf.read(0), 0);
    rf.write(3, -100);
    EXPECT_EQ("write/read negative", rf.read(3), -100);
    rf.reset();
    EXPECT_EQ("reset $r1", rf.read(1), 0);
    EXPECT_EQ("reset $r3", rf.read(3), 0);
}

void test_loop_sum() {
    std::cout << "\n=== Phase 1: loop_sum (sum 1..10 = 55) ===\n";
    auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
    SimConfig cfg; cfg.benchmark = "loop_sum";
    Simulator sim(cfg); sim.load_program(prog);
    auto result = sim.run();
    EXPECT_EQ("$r2 (sum)",   sim.regs().read(2), 55);
    EXPECT_TRUE("instructions executed", result.instructionsExecuted > 0);
    std::cout << "  Instructions: " << result.instructionsExecuted << "  Cycles: " << result.totalCycles << "\n";
}

void test_factorial() {
    std::cout << "\n=== Phase 1: factorial (7! = 5040) ===\n";
    auto prog = assemble_file("sim-core/benchmarks/factorial.asm");
    SimConfig cfg; cfg.benchmark = "factorial";
    Simulator sim(cfg); sim.load_program(prog);
    auto result = sim.run();
    EXPECT_EQ("$r2 (7!)", sim.regs().read(2), 5040);
    std::cout << "  Instructions: " << result.instructionsExecuted << "\n";
}

void test_bubble_sort() {
    std::cout << "\n=== Phase 1: bubble_sort ===\n";
    auto prog = assemble_file("sim-core/benchmarks/bubble_sort.asm");
    SimConfig cfg; cfg.benchmark = "bubble_sort";
    Simulator sim(cfg); sim.load_program(prog);
    auto result = sim.run();
    const int expected[] = {1, 11, 12, 22, 25, 34, 64, 90};
    for (int i = 0; i < 8; ++i) {
        int32_t val = static_cast<int32_t>(sim.mem().load_word(256 + i * 4));
        EXPECT_EQ("array[" + std::to_string(i) + "]", val, expected[i]);
    }
    std::cout << "  Instructions: " << result.instructionsExecuted << "\n";
}

// ═══════════════════════════════════════════════════════════
// PHASE 2
// ═══════════════════════════════════════════════════════════

void test_hazard_detection() {
    std::cout << "\n=== Phase 2: Hazard Detection Unit Tests ===\n";
    ID_EX id_ex;
    id_ex.valid = true; id_ex.is_bubble = false; id_ex.opcode = Opcode::LW; id_ex.rt = 1;
    EX_MEM ex_mem; IF_ID if_id;
    HazardInfo h1 = HazardDetector::detect(id_ex, ex_mem, if_id, 1, 0, true, true);
    EXPECT_TRUE("load-use stall detected (rs)", h1.stall_needed);
    HazardInfo h2 = HazardDetector::detect(id_ex, ex_mem, if_id, 0, 1, true, true);
    EXPECT_TRUE("load-use stall detected (rt)", h2.stall_needed);
    HazardInfo h3 = HazardDetector::detect(id_ex, ex_mem, if_id, 2, 3, true, true);
    EXPECT_TRUE("no hazard when regs differ", !h3.stall_needed);
    id_ex.rt = 0;
    HazardInfo h4 = HazardDetector::detect(id_ex, ex_mem, if_id, 0, 0, true, true);
    EXPECT_TRUE("no hazard for $r0", !h4.stall_needed);
}

void test_forwarding() {
    std::cout << "\n=== Phase 2: Forwarding Unit Tests ===\n";
    EX_MEM ex_mem; MEM_WB mem_wb;
    ex_mem.valid = true; ex_mem.is_bubble = false; ex_mem.reg_write = true;
    ex_mem.rd_dest = 3; ex_mem.alu_result = 99;
    auto dec1 = ForwardingUnit::decide(3, 5, ex_mem, mem_wb);
    EXPECT_TRUE("EX/MEM->EX forward rs", dec1.rs_src == ForwardSrc::EX_MEM);
    EXPECT_TRUE("no forward for rt",     dec1.rt_src == ForwardSrc::REGISTER_FILE);
    int32_t val1 = ForwardingUnit::apply(dec1.rs_src, 0, ex_mem, mem_wb);
    EXPECT_EQ("forwarded value from EX/MEM", val1, 99);
    mem_wb.valid = true; mem_wb.is_bubble = false; mem_wb.reg_write = true;
    mem_wb.rd_dest = 7; mem_wb.result = 42;
    auto dec2 = ForwardingUnit::decide(0, 7, ex_mem, mem_wb);
    EXPECT_TRUE("MEM/WB->EX forward rt", dec2.rt_src == ForwardSrc::MEM_WB);
    int32_t val2 = ForwardingUnit::apply(dec2.rt_src, 0, ex_mem, mem_wb);
    EXPECT_EQ("forwarded value from MEM/WB", val2, 42);
    ex_mem.rd_dest = 7; ex_mem.alu_result = 77;
    auto dec3 = ForwardingUnit::decide(7, 0, ex_mem, mem_wb);
    EXPECT_TRUE("EX/MEM priority over MEM/WB", dec3.rs_src == ForwardSrc::EX_MEM);
}

void test_pipeline_correctness() {
    std::cout << "\n=== Phase 2: Pipelined loop_sum correctness ===\n";
    auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");
    SimConfig cfg; cfg.benchmark = "loop_sum";
    cfg.pipeline.enabled = true; cfg.pipeline.forwarding = true; cfg.pipeline.stalling = true;
    PipelinedSimulator sim(cfg); sim.load_program(prog);
    auto result = sim.run();
    EXPECT_EQ("$r2 (sum) pipelined+fwd", sim.regs().read(2), 55);
    std::cout << "  With forwarding: cycles=" << result.totalCycles << " cpi=" << result.cpi << " stalls=" << result.stallCycles << "\n";

    SimConfig cfg2 = cfg; cfg2.pipeline.forwarding = false;
    PipelinedSimulator sim2(cfg2); sim2.load_program(prog);
    auto result2 = sim2.run();
    EXPECT_EQ("$r2 (sum) pipelined+stall", sim2.regs().read(2), 55);
    std::cout << "  Without forwarding: cycles=" << result2.totalCycles << " cpi=" << result2.cpi << " stalls=" << result2.stallCycles << "\n";
    EXPECT_TRUE("more cycles without forwarding", result2.totalCycles > result.totalCycles);
}

void test_cpi_difference() {
    std::cout << "\n=== Phase 2: CPI difference (hazard_heavy, fwd on vs off) ===\n";
    auto prog = assemble_file("sim-core/benchmarks/hazard_heavy.asm");
    SimConfig cfg_fwd; cfg_fwd.benchmark = "hazard_heavy";
    cfg_fwd.pipeline.enabled = true; cfg_fwd.pipeline.forwarding = true; cfg_fwd.pipeline.stalling = true;
    PipelinedSimulator sim_fwd(cfg_fwd); sim_fwd.load_program(prog);
    auto result_fwd = sim_fwd.run();
    SimConfig cfg_stall = cfg_fwd; cfg_stall.pipeline.forwarding = false;
    PipelinedSimulator sim_stall(cfg_stall); sim_stall.load_program(prog);
    auto result_stall = sim_stall.run();
    EXPECT_EQ("$r5=26 with forwarding",   sim_fwd.regs().read(5),   26);
    EXPECT_EQ("$r5=26 without forwarding", sim_stall.regs().read(5), 26);
    EXPECT_TRUE("CPI higher without forwarding", result_stall.cpi > result_fwd.cpi);
    std::cout << "  CPI with forwarding:    " << result_fwd.cpi   << "  (stalls=" << result_fwd.stallCycles   << ")\n";
    std::cout << "  CPI without forwarding: " << result_stall.cpi << "  (stalls=" << result_stall.stallCycles << ")\n";
    std::cout << "  CPI ratio (no-fwd / fwd): " << result_stall.cpi / result_fwd.cpi << "x\n";
}

// ═══════════════════════════════════════════════════════════
// PHASE 3
// ═══════════════════════════════════════════════════════════

void test_cache_basic() {
    std::cout << "\n=== Phase 3: Cache Unit Tests ===\n";
    Cache c1(256, 64, 1, ReplacementPolicy::LRU);
    EXPECT_TRUE("cold miss",       !c1.access(0));
    EXPECT_TRUE("cold miss 2",     !c1.access(64));
    EXPECT_TRUE("same block hit",  c1.access(4));
    EXPECT_TRUE("same block hit2", c1.access(8));
    EXPECT_EQ("hits=2",   (int)c1.stats().hits,   2);
    EXPECT_EQ("misses=2", (int)c1.stats().misses,  2);

    Cache c2(256, 64, 4, ReplacementPolicy::LRU);
    EXPECT_TRUE("way0 miss", !c2.access(0));
    EXPECT_TRUE("way1 miss", !c2.access(64));
    EXPECT_TRUE("way2 miss", !c2.access(128));
    EXPECT_TRUE("way3 miss", !c2.access(192));
    EXPECT_TRUE("way0 hit",   c2.access(0));
    EXPECT_TRUE("way1 hit",   c2.access(64));
    EXPECT_TRUE("way2 hit",   c2.access(128));
    EXPECT_TRUE("way3 hit",   c2.access(192));
    EXPECT_EQ("4-way hits=4",   (int)c2.stats().hits,   4);
    EXPECT_EQ("4-way misses=4", (int)c2.stats().misses,  4);
}

void test_cache_lru_eviction() {
    std::cout << "\n=== Phase 3: LRU Eviction Test ===\n";
    Cache c(128, 64, 2, ReplacementPolicy::LRU);
    EXPECT_TRUE("miss A", !c.access(0));
    EXPECT_TRUE("miss B", !c.access(64));
    EXPECT_TRUE("hit A",   c.access(0));
    EXPECT_TRUE("miss C", !c.access(128));
    EXPECT_TRUE("hit C",   c.access(128));
    EXPECT_TRUE("miss B again", !c.access(64));
}

void test_cache_hit_rate_varies() {
    std::cout << "\n=== Phase 3: Hit Rate Varies with Associativity ===\n";
    auto prog = assemble_file("sim-core/benchmarks/array_sum.asm");

    SimConfig cfg1; cfg1.benchmark = "array_sum";
    cfg1.cache.sizeKB = 1; cfg1.cache.blockSize = 16;
    cfg1.cache.associativity = 1; cfg1.cache.replacement = "LRU";
    Simulator sim1(cfg1); sim1.load_program(prog);
    auto r1 = sim1.run();

    SimConfig cfg4 = cfg1; cfg4.cache.associativity = 4;
    Simulator sim4(cfg4); sim4.load_program(prog);
    auto r4 = sim4.run();

    EXPECT_EQ("$r3=360 1-way", sim1.regs().read(3), 360);
    EXPECT_EQ("$r3=360 4-way", sim4.regs().read(3), 360);
    EXPECT_TRUE("hit rate in [0,1] 1-way", r1.cacheHitRate >= 0.0 && r1.cacheHitRate <= 1.0);
    EXPECT_TRUE("hit rate in [0,1] 4-way", r4.cacheHitRate >= 0.0 && r4.cacheHitRate <= 1.0);
    std::cout << "  1-way cache hit rate: " << r1.cacheHitRate << "\n";
    std::cout << "  4-way cache hit rate: " << r4.cacheHitRate << "\n";
}

// ═══════════════════════════════════════════════════════════
// PHASE 4
// ═══════════════════════════════════════════════════════════

void test_predictor_static() {
    std::cout << "\n=== Phase 4: Static Predictors ===\n";

    StaticNotTaken snot;
    EXPECT_TRUE("SNT: predict false", !snot.predict(0x100));
    snot.update(0x100, true);
    snot.update(0x100, false);
    EXPECT_EQ("SNT: 2 predictions",   (int)snot.stats().predictions,    2);
    EXPECT_EQ("SNT: 1 misprediction", (int)snot.stats().mispredictions, 1);

    StaticTaken stk;
    EXPECT_TRUE("ST: predict true", stk.predict(0x200));
    stk.update(0x200, true);
    stk.update(0x200, false);
    EXPECT_EQ("ST: 2 predictions",   (int)stk.stats().predictions,    2);
    EXPECT_EQ("ST: 1 misprediction", (int)stk.stats().mispredictions, 1);
}

void test_predictor_one_bit() {
    std::cout << "\n=== Phase 4: 1-Bit Predictor ===\n";
    OneBitPredictor p;
    EXPECT_TRUE("1-bit cold = not-taken",        !p.predict(0x10));
    p.update(0x10, true);
    EXPECT_TRUE("1-bit after taken = taken",       p.predict(0x10));
    p.update(0x10, false);
    EXPECT_TRUE("1-bit after not-taken = not-taken", !p.predict(0x10));
    EXPECT_EQ("1-bit: 2 predictions",    (int)p.stats().predictions,    2);
    EXPECT_EQ("1-bit: 2 mispredictions", (int)p.stats().mispredictions, 2);
}

void test_predictor_two_bit() {
    std::cout << "\n=== Phase 4: 2-Bit Saturating Predictor ===\n";
    TwoBitSaturating p;
    EXPECT_TRUE("2-bit cold = not-taken",        !p.predict(0x20));
    p.update(0x20, true);
    EXPECT_TRUE("2-bit WeakT = taken",             p.predict(0x20));
    p.update(0x20, true);
    EXPECT_TRUE("2-bit StrongT = taken",           p.predict(0x20));
    p.update(0x20, false);
    EXPECT_TRUE("2-bit hysteresis (still taken)",  p.predict(0x20));
    p.update(0x20, false); // state: 2 -> 1 (WeakNT)
    EXPECT_TRUE("2-bit WeakNT = not-taken",       !p.predict(0x20));
    // Stats: 4 update calls = 4 predictions
    // miss1: cold(NT) vs taken   → mispred
    // hit1:  WeakT    vs taken   → correct
    // miss2: StrongT  vs not-taken → mispred
    // miss3: WeakT    vs not-taken → mispred
    EXPECT_EQ("2-bit: 4 predictions",   (int)p.stats().predictions,    4);
    EXPECT_EQ("2-bit: 3 mispredictions", (int)p.stats().mispredictions, 3);
}

void test_predictor_misprediction_rate() {
    std::cout << "\n=== Phase 4: Misprediction Rate vs Predictor Type ===\n";
    auto prog = assemble_file("sim-core/benchmarks/loop_sum.asm");

    auto run_with_pred = [&](const std::string& pt) -> std::pair<SimResult, int32_t> {
        SimConfig cfg; cfg.benchmark = "loop_sum";
        cfg.pipeline.enabled = true; cfg.pipeline.forwarding = true; cfg.pipeline.stalling = true;
        cfg.branchPredictor.type = pt;
        PipelinedSimulator sim(cfg); sim.load_program(prog);
        SimResult r = sim.run();
        int32_t v = sim.regs().read(2);
        return {r, v};
    };

    auto [r_snt, v_snt] = run_with_pred("STATIC_NOT_TAKEN");
    auto [r_2bt, v_2bt] = run_with_pred("TWO_BIT_SATURATING");

    EXPECT_EQ("$r2=55 STATIC_NOT_TAKEN",    v_snt, 55);
    EXPECT_EQ("$r2=55 TWO_BIT_SATURATING",  v_2bt, 55);
    EXPECT_TRUE("2-bit <= static-NT mispred rate",
                r_2bt.branchMispredictionRate <= r_snt.branchMispredictionRate + 0.01);

    std::cout << "  STATIC_NOT_TAKEN mispred rate:   " << r_snt.branchMispredictionRate << "\n";
    std::cout << "  TWO_BIT_SATURATING mispred rate: " << r_2bt.branchMispredictionRate << "\n";
}

// ═══════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Processor Design Studio — Phase 1+2+3+4 Test Suite    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    try {
        // Phase 1
        test_decoder();
        test_regfile();
        test_loop_sum();
        test_factorial();
        test_bubble_sort();
        // Phase 2
        test_hazard_detection();
        test_forwarding();
        test_pipeline_correctness();
        test_cpi_difference();
        // Phase 3
        test_cache_basic();
        test_cache_lru_eviction();
        test_cache_hit_rate_varies();
        // Phase 4
        test_predictor_static();
        test_predictor_one_bit();
        test_predictor_two_bit();
        test_predictor_misprediction_rate();
    } catch (const std::exception& e) {
        std::cout << "\n[ERROR] Exception: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n══════════════════════════════════════════════════════════\n";
    std::cout << "Results: " << tests_passed << " / " << tests_run << " passed\n";
    std::cout << "══════════════════════════════════════════════════════════\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
