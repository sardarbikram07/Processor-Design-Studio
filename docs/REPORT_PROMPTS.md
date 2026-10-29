# Processor Design Studio -- Report Generation Prompts
> Author: Bikram Sardar | Reg No: 26MAI0065 | VIT Vellore, M.Tech CSE

---

## REVIEW STAGE DIVISION

| Stage | Scope |
|---|---|
| Review 1 | Problem, literature survey, proposed architecture + ALL full results |
| Review 2 | Deep-dive implementation: HOW each module was built |
| Review 3 | Full-stack, OpenMP DSE, Amdahl analysis, 294-config sweep, comparison |
| Final Report | Merge all 3 reviews -- IEEE chapters + IPR patent specification |

---

## SCREENSHOTS REFERENCE

All screenshots in: D:\Desktop\Processor design studio\images\screenshots\

| File | Description |
|---|---|
| 01_home_landing.png | App home with Config Builder |
| 02_config_builder.png | Full parameter form |
| 03_config_result.png | CPI=1.581, IPC=0.632, Stalls=0 |
| 04_pipeline_visualizer.png | Pipeline initial state |
| 05_pipeline_active.png | Pipeline at Cycle 4/8 |
| 06_dashboard_before_sweep.png | Dashboard before sweep |
| 07_dashboard_after_sweep.png | 294-config: CPI_min=1.000, mean=1.684 |

---


## KEY DATA (common to all prompts)

**Benchmarks:** loop_sum(44 instr), factorial(48), bubble_sort(164), array_sum(56), hazard_heavy(34), matrix_multiply(382).
**Tests:** 253 total, 16 groups, 100% pass. Groups: Decoder(12), RegFile(5), ALU(12), Memory(4), Control(10), Assembler(5), Benchmarks(6), Latches(10), Hazards(7), Forwarding(7), CPI(5), Cache(10), CacheInteg(5), Predictor(9), PredInteg(5), Edge(12).
**Key Results:** Forwarding: 45.7% CPI reduction on hazard_heavy (2.382->1.294). Cache: LRU 93.1% vs FIFO 90.3% vs Random 85.9%. Predictor: TWO_BIT_SAT 9.09% miss. OpenMP: 1.48x speedup at 2 threads, s=35%, S_max=2.85x. Sweep: 294 configs, CPI_min=1.000, CPI_mean=1.684.

**References [1]-[11]:**
[1] Hwang et al., Sim. Modelling Practice Theory, vol.138, 2025. doi:10.1016/j.simpat.2024.103032
[2] Mhatre & Chandran, IEEE Open J. Comput. Soc., 2025. doi:10.1109/OJCS.2025.3591359
[3] Mariotti & Giorgi, SoftwareX, vol.18, 2022.
[4] Grosse et al., IEEE Access, vol.14, 2026.
[5] Giorgi, Proc. WCAE 2025.
[6] Muttillo et al., EURASIP J. Embedded Systems, 2016.
[7] US Patent 2016/0063164 A1
[8] US Patent 7,788,078 B1 (Synopsys)
[9] US Patent 9,928,179 B2 (Intel)
[10] US Patent 6,263,427 B1 (Rise Technology)
[11] gem5 Project. https://www.gem5.org

---

## PROMPT 1: REVIEW 1 (IEEE Format)

Paste into Claude/GPT-4o/Gemini. Generate completely without truncation.

**FORMATTING:** Black and white only. No colors. No shaded table cells. IEEE two-column layout. Equations numbered (1),(2)... IEEE citations [1]-[11]. 6-8 IEEE pages.

**TITLE:** "Processor Design Studio: Problem Formulation, Literature Survey, and Full-Stack Microarchitecture Simulation Platform -- Review 1"
**AUTHOR:** Bikram Sardar, Reg. No. 26MAI0065, M.Tech CSE, VIT Vellore-632014, India.

**SECTIONS TO GENERATE:**

ABSTRACT (150 words): Platform overview. Motivated by USD 50M+ tape-out costs, 60% re-spin rates, no parallel DSE tools. Surveyed 6 papers and 4 patents. 253 tests pass (100%). Forwarding reduces CPI 45.7%. 4-way LRU 93.1% hit rate. OpenMP 1.48x speedup, s=35%.

SECTION I: INTRODUCTION -- 6 paragraphs: (1) Post-Moore context; (2) USD 50M-100M+ tape-out costs, 60% re-spin rates; (3) observability gap; (4) pedagogy gap; (5) three use cases (DSE, education, compiler); (6) paper organization.

SECTION II: LITERATURE SURVEY -- II-A: 6 papers with full citation + gap. II-B: 4 patents with gap. II-C: B&W feature matrix (7 features x 6 tools, Yes/No/Partial).

SECTION III: RESEARCH GAP -- No tool combines all 5 features simultaneously.

SECTION IV: METHODOLOGY -- IV-A: 3-tier architecture. IV-B: 32-bit RISC ISA (19 instructions, R/I/J formats, 16 GPRs). IV-C: Pipeline equations (1)-(3). IV-D: Cache equations (4)-(6). IV-E: Branch predictor FSM. IV-F: OpenMP sweep code (Eq.7).

SECTION V: RESULTS -- V-A: Setup. V-B: Benchmark table (6 rows). V-C: Test table (16 groups, 253 tests). V-D: Forwarding/stall table (5 rows). V-E: Cache table (5 rows). V-F: Predictor table (4 rows). V-G: Amdahl table + full algebra (Eq.8-10).

SECTION VI: WEB INTERFACE -- 4 figures with captions: Fig.1=02_config_builder.png, Fig.2=03_config_result.png, Fig.3=05_pipeline_active.png, Fig.4=07_dashboard_after_sweep.png.

SECTION VII: LIMITATIONS AND FUTURE WORK
SECTION VIII: CONCLUSION
REFERENCES [1]-[11]

---

## PROMPT 2: REVIEW 2 (IEEE Format)

**TITLE:** "Processor Design Studio -- Review 2: Implementation of a Cycle-Accurate C++17 RISC Pipeline Engine with Configurable Cache and Branch Prediction"
**AUTHOR:** Bikram Sardar, Reg. No. 26MAI0065, M.Tech CSE, VIT Vellore, India.
**FORMATTING:** Same as Prompt 1. 6-8 IEEE pages. No truncation.

**CONTEXT:** Review 1 = problem+survey+proposal. Review 2 = implementation internals of each module. All results from actual C++17 code.

**ABSTRACT (150 words):** Review 2 of Processor Design Studio. Details C++17 implementation of ISA, two-pass assembler, latch-snapshot 5-stage pipeline, hazard/forwarding, L1 cache (LRU/FIFO/Random), 4-mode branch predictor with 2-bit FSM. 16 test groups, 253 tests, 100% pass. Forwarding: CPI -45.7%. LRU: 93.1% hit rate. Pipeline Visualizer: cycle-by-cycle React interface.

**SECTIONS:**

SECTION I: INTRODUCTION -- Context from Review 1. Review 2 scope: actual C++17 code drives all results.

SECTION II: ISA AND ASSEMBLER -- II-A: Full ISA table (21 rows, B&W): ADD/SUB/AND/OR/XOR/SLL/SRL/JR (R-type, 0x00), ADDI/ANDI/ORI/LW/SW/BEQ/BNE/BLT/BGE (I-type, 0x01-0x09), J/JAL (J-type, 0x0A-0x0B), NOP(0x3E), HALT(0x3F). Columns: Mnemonic|Format|Opcode|Funct|Operation. II-B: Two-pass assembler + encoding example "ADDI , , 10".

SECTION III: 5-STAGE PIPELINE -- III-A: Latch-snapshot model Eq(1). III-B: Stage descriptions (5 paragraphs). III-C: Hazard detection Eq(2). III-D: Forwarding unit Eq(3), Fig.1=05_pipeline_active.png.

SECTION IV: L1 CACHE -- IV-A: Geometry Eq(4)-(6). IV-B: 3 eviction policies. IV-C: Write policy. IV-D: Performance table (5 rows, B&W).

SECTION V: BRANCH PREDICTION -- V-A: 4 predictors + 2-bit FSM Eq(7). V-B: Recovery. V-C: Accuracy table (4 rows, B&W).

SECTION VI: PIPELINE VISUALIZER -- 3-panel layout. Fig.2=05_pipeline_active.png, Fig.3=02_config_builder.png.

SECTION VII: VERIFICATION RESULTS -- 16-group 253-test table (B&W). Highlight groups 8-10, 12-14.

SECTION VIII: CONCLUSION -- Module correctness summary. Preview Review 3.

REFERENCES [1]-[11]

---

## PROMPT 3: REVIEW 3 (IEEE Format)

**TITLE:** "Processor Design Studio -- Review 3: OpenMP-Parallelized Design-Space Exploration, Full-Stack Integration, and Comprehensive Architectural Evaluation"
**AUTHOR:** Bikram Sardar, Reg. No. 26MAI0065, M.Tech CSE, VIT Vellore, India.
**FORMATTING:** Same rules. 6-8 pages. No truncation.

**ABSTRACT (150 words):** Review 3 covers OpenMP parallel sweep + Amdahl analysis + full-stack REST+React + 294-config evaluation + tool comparison. s=35%, S_max=2.85x. CPI_min=1.000, CPI_mean=1.684. 8-feature comparison: Proposed Studio is only tool with all 8.

**SECTIONS:**

SECTION I: 5 contributions of Review 3.

SECTION II: OPENMP SWEEP -- II-A: 294-config grid (6 benchmarks x 3 pipeline x 5 cache x 4 predictors). II-B: Parallel code Eq(1), thread isolation, dynamic chunk=1. II-C: JSON aggregation.

SECTION III: AMDAHL'S LAW -- III-A: Scaling table (4 rows, B&W: 1/2/4/8 threads). III-B: Full algebra Eq(2)-(4): s=0.350, S_max=2.85x. III-C: 4 bottleneck components.

SECTION IV: FULL-STACK -- IV-A: REST API endpoint table (4 endpoints, B&W). Subprocess pattern Eq(5). IV-B: 3 React pages described. Figs 1-4: 02_config_builder, 03_config_result, 05_pipeline_active, 07_dashboard_after_sweep.

SECTION V: RESULTS -- V-A: 6-benchmark CPI table (functional/stall/forward). V-B: 3 key findings. V-C: 294-config summary table.

SECTION VI: COMPARISON -- B&W table 8 features x 5 tools (gem5/WebRISC-V/SonicRV/FREESS/Proposed). Proposed is only tool with all 8 features.

SECTION VII: CONCLUSION. REFERENCES [1]-[11].

---

## PROMPT 4: FINAL REPORT (IEEE + IPR Format)

**TITLE:** "Processor Design Studio: A Unified Full-Stack Platform for Cycle-Accurate Microarchitecture Simulation, Parallel Design-Space Exploration, and Interactive Pipeline Observability -- Final Project Report"
**AUTHOR:** Bikram Sardar | Reg.: 26MAI0065 | M.Tech CSE | VIT Vellore | October 2026
**FORMATTING:** BLACK AND WHITE ONLY. No colors anywhere. IEEE two-column for Chapters 3-6. Single-column for Part A and Part C. Generate ALL sections completely.

**PART A: PRELIMINARY PAGES**
A1. Title Page (formal academic, logo placeholder, all metadata).
A2. Certificate of Approval (bonafide certificate, signature spaces).
A3. Student Declaration (originality, no plagiarism).
A4. Acknowledgements (supervisor, department, open-source tools).
A5. Abstract (250 words -- full comprehensive).
A6. Table of Contents. A7. List of Tables. A8. List of Figures.
A9. List of Abbreviations: RISC, ISA, CPI, IPC, DSE, LRU, FIFO, RAW, ALU, FSM, REST, API, JSON, OpenMP, SoC, GPR, RTL, EDA, MESI.

**PART B: MAIN TECHNICAL REPORT**
Chapter 1: Introduction (1.1 Background, 1.2 Problem Statement, 1.3 Objectives, 1.4 Contributions, 1.5 Organization)
Chapter 2: Literature Survey (2.1 six papers, 2.2 four patents, 2.3 feature matrix, 2.4 gap)
Chapter 3: System Architecture (3.1-3.6 covering ISA, assembler, pipeline, cache, predictor)
Chapter 4: Parallel DSE (4.1-4.5 covering sweep, OpenMP, thread isolation, REST API, React)
Chapter 5: Experimental Results (5.1-5.9 with all B&W tables and 4 screenshot figures)
Chapter 6: State-of-the-Art Comparison (B&W 8-feature x 5-tool table + analysis)
Chapter 7: Limitations and Future Work
Chapter 8: Conclusion
References [1]-[11]

**PART C: IPR PATENT SPECIFICATION**

C1. TITLE: "A FULL-STACK SYSTEM AND METHOD FOR PARALLEL MULTI-PARAMETER MICROARCHITECTURE DESIGN-SPACE EXPLORATION AND CYCLE-ACCURATE PIPELINE VISUALIZATION"

C2. FIELD: Computer architecture simulation, EDA, parallel computation, interactive visualization.

C3. BACKGROUND: Prior art deficiencies (gem5, WebRISC-V, SonicRV, FREESS, Muttillo 2016, US 7,788,078 B1).

C4. SUMMARY: Three-tier system overview.

C5. DRAWINGS: 8 figures described (B&W block diagrams + 3 UI screenshots).

C6. DETAILED DESCRIPTION: 9 embodiment paragraphs (ISA, assembler, latch-snapshot, stall detection, forwarding, cache, predictor, OpenMP, React UI).

C7. CLAIMS (10 formal legal claims -- write each in full legal language):
Claim 1 (System): 5-stage pipeline + OpenMP sweep + REST API + web UI.
Claim 2 (Latch-snapshot synchronization mechanism).
Claim 3 (EX/MEM and MEM/WB forwarding with priority).
Claim 4 (Configurable cache: 1-8 way, 1KB-64KB, LRU/FIFO/Random).
Claim 5 (4-mode branch predictor with 2-bit FSM + 2-cycle flush).
Claim 6 (Method): Parallel DSE grid construction and execution.
Claim 7 (Thread isolation via stack allocation).
Claim 8 (Serial fraction s bounds speedup to 1/s).
Claim 9 (Cycle-by-cycle pipeline state player with PREV/NEXT navigation).
Claim 10 (Automated verification suite >= 253 tests, 16 categories).

C8. ABSTRACT OF DISCLOSURE (150 words): Three-tier system. 1.48x speedup, s=35%, S_max=2.85x. 253 tests, 100% pass. CPI -45.7%. 93.1% cache hit. 294-config sweep.

**IMPORTANT:** Generate this entire document completely. Do not truncate any chapter, section, table, equation, claim, or description.
