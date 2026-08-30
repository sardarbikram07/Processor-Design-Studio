Processor Design Studio
A Unified Platform for Processor Simulation, Architecture Exploration, and Comparative Performance Analysis
Review-1 Submission Document
Prepared by: Bikarm Sardar
Registration Number: 26MAI0065
Domain: Computer Architecture, Microarchitecture Simulation, Parallel Computing
 
1. Substantiation of Problem with Justification
1.1 Background
With the physical decay of Dennard Scaling and the slowdown of Moore's Law, processor performance gains can no longer be achieved simply by scaling clock frequencies or shrinking transistor dimensions; gains increasingly require domain-specific architectures, custom instruction extensions, and intricate memory-hierarchy optimization. Evaluating such changes via physical silicon tape-outs or HDL (Verilog/VHDL) is expensive and slow:
●	Prototyping cost: fabricating a modern IC prototype at advanced nodes (sub-7nm) runs into the tens of millions of dollars per iteration, with total design costs at leading-edge nodes reported well above $100M by industry cost analyses (IBS, IDC).
●	Re-spin risk: a substantial share of commercial SoC revisions require costly hardware re-spins due to microarchitectural bottlenecks — pipeline stalls, cache thrashing, branch misprediction — undetected before synthesis.
●	Observability: physical silicon and FPGA prototypes provide minimal visibility into pipeline latches, hazard conditions, or cycle-by-cycle cache behaviour.
1.2 Why Now, and Why Software
●	Open ISAs (RISC-V) and heterogeneous/multi-core workloads increase demand for accessible, parallel-capable simulation environments over slow full-system simulators (e.g. gem5, which runs orders of magnitude slower than physical execution).
●	Software simulation gives full observability (every latch, register, cache line, every cycle), instant reconfigurability (no re-synthesis), and parallel exploration of many configurations at once via OpenMP.
1.3 Primary Use Cases
●	Design-space exploration: sweeping cache size/associativity/replacement policy and branch-predictor choice to study power-performance-area trade-offs before RTL is written.
●	Pedagogy: letting students trace instructions through the 5-stage pipeline cycle by cycle, observing stalling versus forwarding.
●	Compiler/firmware study: evaluating assembly-level optimizations against RAW hazards and control-branch penalties.
2. Survey — Comparative Analysis of Existing Solutions
2.1 Academic Publications
#	Title	Authors, Year, Venue	Access	Gap Identified
1	Survey of CPU and Memory Simulators in Computer Architecture	Hwang et al., 2025, Simulation Modelling Practice and Theory (Elsevier), DOI 10.1016/j.simpat.2024.103032	Licensed (RightsLink #6336060626802)	Full-system simulators have steep setup complexity, no real-time interactive UI
2	Simulators for Processors Used in Virtualization: A Survey	Mhatre & Chandran, 2025, IEEE Open Journal of the Computer Society, DOI 10.1109/OJCS.2025.3591359	Open Access	No native parallel-execution framework for multi-parameter sweeps
3	WebRISC-V: A 32/64-bit RISC-V pipeline simulation tool	Mariotti & Giorgi, 2022, SoftwareX (Elsevier), DOI 10.1016/j.softx.2022.101105	Open Access (CC BY)	Web pipeline visualiser only — one config at a time, no cache/predictor sweep, no OpenMP DSE
4	SonicRV: An Educational Platform for Web-Based Simulation and Visualization of RISC-V Processor Architectures	Große, Reitinger & Klemmer, 2026, IEEE Access, DOI 10.1109/ACCESS.2026.3656022	Open Access	Single/multicycle/pipeline visualiser; no architecture-parameter sweep or parallel exploration
5	FREESS: An Educational Simulator of a RISC-V-Inspired Superscalar Processor Based on Tomasulo's Algorithm	Giorgi, 2025, WCAE'25 (ACM), DOI 10.1145/3743646.3750018 / arXiv:2506.07665	Open Access	Superscalar/Tomasulo visualiser for one configuration; no cache/branch-predictor sweep or parallel DSE
6	A design methodology for soft-core platforms on FPGA with SMP Linux, OpenMP support, and distributed hardware profiling system	Muttillo et al., 2016, EURASIP Journal on Embedded Systems, DOI 10.1186/s13639-016-0051-9	Open Access (CC BY 4.0)	OpenMP-based DSE, but for FPGA soft-core hardware synthesis — not an ISA-level pipeline/cache/predictor simulator with a web UI

2.2 Patent Filings
Patent	Title	Assignee	Gap Identified
US 2016/0063164 A1	Method and System for Hardware/Software Co-simulation and Processor Resource Allocation	—	Host-level scheduling only, no microarchitectural component-level observability
US 7,788,078 B1	Processor/Memory Co-Exploration at Multiple Abstraction Levels	CoWare Inc. / Synopsys Inc.	Proprietary ADL toolchain; no interactive visualisation or parallel sweep
US 9,928,179 B2	Cache Replacement Policy	Intel Corporation	Hardware replacement-policy mechanism; not a configurable simulation/exploration tool
US 6,263,427 B1	Branch Prediction Mechanism	Rise Technology Co.	Hardware branch-target-buffer design; not simulated or comparatively configurable

2.3 Existing Software Products / Tools
gem5:
Modular full-system research platform. Cons — high complexity, steep learning curve, CLI-only, no interactive pipeline playback.
SimpleScalar / MARSSx86:
Legacy academic toolsets. Cons — largely unmaintained, no multi-core parallel sweep capability, no modern web UI.
WebRISC-V / SonicRV / FREESS (tool form of Publications 3–5 above):
Strong cycle-by-cycle pipeline or superscalar visualisation in the browser. Cons — each evaluates one architecture configuration at a time; none sweep across cache, associativity, or branch-predictor parameters, and none parallelise that sweep.



3. Comparative Analysis & Gap Identification
Reference	Key Conclusion / Gap
Hwang et al. (2025)	Existing tools trade simulation speed against microarchitectural accuracy; usability is often secondary.
Mhatre & Chandran (2025)	Tools lack native parallel frameworks for multi-parameter sweeps.
WebRISC-V / SonicRV / FREESS	Excellent single-configuration visualisation; no parameter sweep, no parallel exploration.
Muttillo et al. (2016)	OpenMP-based DSE proven effective, but for FPGA hardware synthesis, not ISA-level software simulation.
US 7,788,078 B1 / US 9,928,179 B2 / US 6,263,427 B1	Real ADL, cache, and branch-predictor mechanisms, but fixed hardware designs, not comparative exploration tools.
gem5	Industry-accurate but heavy, complex, and not visually interactive.
Proposed: Processor Design Studio	Combines cycle-accurate simulation, parallel OpenMP sweeps, and web visualisation.

Feature	Lit. / Patents	Educational Tools*	gem5	Proposed Studio
5-stage cycle-accurate pipeline	Partial	Yes	Yes	Planned
RAW hazard detection & forwarding	Conceptual	Yes	Yes	Planned, toggleable
Configurable cache (LRU/FIFO/Rand)	Yes	No	Yes	Planned
Dynamic branch prediction	Yes	Partial	Yes	Planned, 4 types
Parameter sweep across configs	No	No	Manual only	Planned
OpenMP-parallelised sweep	No / thread-alloc only	No	No (manual MPI)	Planned, native OpenMP
Interactive web UI	No	Yes	No	Planned, React
Open-source, zero cost	Mostly proprietary	Yes	Open-source	Planned, open-source
*WebRISC-V, SonicRV, FREESS.
3.1 Identified Gap and Proposed Contribution
No single publication, patent, or tool in this survey combines all four of: (a) a cycle-accurate pipeline with toggleable hazard detection and forwarding, (b) a configurable cache hierarchy across multiple replacement policies, (c) multiple branch-predictor models, and (d) an OpenMP-parallelised sweep across those configurations surfaced through an interactive web dashboard. The educational visualisers (WebRISC-V, SonicRV, FREESS) are strong at (a)–(c) for one configuration at a time but do not sweep or parallelise across the design space. Research and vendor tools (gem5; the CoWare/Synopsys ADL flow) support configuration exploration but are heavyweight, CLI-driven, or proprietary, with no comparison dashboard. The one directly comparable OpenMP-driven design-space-exploration methodology found (Muttillo et al., 2016) applies the same idea — simulate, sweep, parallelise — but to FPGA soft-core hardware synthesis, not to ISA-level software simulation of pipeline/cache/predictor trade-offs.
This is the gap Processor Design Studio is proposed to fill. The new contribution is the combination itself, not any single component in isolation: an OpenMP-parallelised sweep runner (Section 4.2) driving a cycle-accurate simulation core, with results surfaced as directly comparable charts in a web dashboard — turning a manual, one-configuration-at-a-time exercise (as in the closest existing tools) into a parallel, multi-configuration exploration workflow.
4. Proposed Methodology
Three decoupled layers are proposed: a C++17/OpenMP simulation core, a Spring Boot backend invoking it as a subprocess, and a React frontend for configuration and visualisation. The core requires OpenMP (hence C/C++); the backend and UI favour faster iteration in Java and JavaScript.
[ Assembly (.asm) ] -> [ Two-Pass Assembler ] -> [ 32-bit Machine Code ]
[ Config (JSON) ] -> [ 5-Stage Pipelined Sim ] -> Cache | Branch Predictor | OpenMP Sweep
[ Visual Dashboard ] <- [ REST API ] <- [ SimResult JSON Output ]
4.1 Planned Algorithms
●	Two-pass assembler: pass 1 builds a label-to-address table; pass 2 emits 32-bit R-type/I-type/J-type encoded instructions.
●	Latch-snapshot pipeline: each stage reads a snapshot latch and writes a next-cycle latch, avoiding intra-cycle read-after-write anomalies.
●	Hazard/forwarding: load-use stall triggers when an ID/EX load's destination matches an IF/ID source register; EX/MEM->EX and MEM/WB->EX bypass paths planned, priority to the younger producer.
●	Cache: standard set-index/tag computation; planned LRU, FIFO, and Random eviction policies.
●	Branch prediction: static baselines plus a per-PC 2-bit saturating counter; 2-stage flush on misprediction.
●	OpenMP sweep: independent configs distributed via #pragma omp parallel for schedule(dynamic,1) — no shared mutable state across threads.
5. Planned Experimental Setup
●	Compiler: GCC/MinGW-w64 (C++17), -O2 -fopenmp -Wall -Wextra  |  Build: CMake  |  Backend: Java 17 / Spring Boot / Maven  |  Frontend: React 18 / Vite / Recharts
Benchmark	Workload	Instruction Mix
loop_sum.asm	Tight counter loop	ADDI, ADD, BEQ, J, HALT
factorial.asm	Recursive calls	JAL, JR, LW, SW, ADDI, SUB
bubble_sort.asm	In-place sort (10 elems)	LW, SW, BLT, BGE, ADDI, ADD
array_sum.asm	Linear memory traversal	LW, ADD, ADDI, BNE
hazard_heavy.asm	Dense RAW/load-use hazards	Dependent ADD, LW, SUB
matrix_multiply.asm	3x3 matrix multiply	Nested loops; LW, SW, SLL, BEQ
6. Expected Outcomes & Evaluation Plan
Implementation follows this review, so no empirical results are reported here. The table below fixes what will be measured and the hypothesis each measurement tests, so results can be reported against this plan in the next review.
Category	What Will Be Measured	Hypothesis to Test
Pipeline	Cycles, CPI, IPC, stalls per benchmark/config	Forwarding reduces CPI vs. stall-only on hazard-dense code
Cache	Hit rate, misses per size/associativity/policy	Higher associativity + LRU improves hit rate, with diminishing returns
Branch prediction	Misprediction rate per predictor type	2-bit saturating matches/beats static on loops, wins on irregular branches
OpenMP scaling	Wall time, speedup, efficiency at 1/2/4/8 threads	Serial fraction (I/O, aggregation) bounds speedup per Amdahl's Law

Results will be presented as comparison tables and charts generated by the React dashboard, included in the subsequent review once implementation is complete.


7. Anticipated Limitations & Future Work
●	Limitation: toy ~19-instruction MIPS-style ISA, not full RV32I / floating point.
●	Limitation: L1-only memory hierarchy; L2/L3 and main-memory wait states out of scope this phase.
●	Limitation: in-order 5-stage pipeline; no out-of-order execution (Tomasulo/ROB) this phase.
●	Future work: extend to full RV32I/RV64I; add multi-level cache and MESI/MOESI coherence; add out-of-order execution; compile the core to WebAssembly for client-side-only execution.
References
[1] Hwang, I., Lee, J., Kang, H., Lee, G., & Kim, H. (2025). Survey of CPU and Memory Simulators in Computer Architecture. Simulation Modelling Practice and Theory, 138, 103032. https://doi.org/10.1016/j.simpat.2024.103032
[2] Mhatre, S. C., & Chandran, P. (2025). Simulators for Processors Used in Virtualization: A Survey. IEEE Open Journal of the Computer Society. https://doi.org/10.1109/OJCS.2025.3591359
[3] Mariotti, G., & Giorgi, R. (2022). WebRISC-V: A 32/64-bit RISC-V pipeline simulation tool. SoftwareX, 18, 101105. https://doi.org/10.1016/j.softx.2022.101105
[4] Große, D., Reitinger, S., & Klemmer, L. (2026). SonicRV: An Educational Platform for Web-Based Simulation and Visualization of RISC-V Processor Architectures. IEEE Access, 14, 13849-13864. https://doi.org/10.1109/ACCESS.2026.3656022
[5] Giorgi, R. (2025). FREESS: An Educational Simulator of a RISC-V-Inspired Superscalar Processor Based on Tomasulo's Algorithm. WCAE '25. https://doi.org/10.1145/3743646.3750018 (also arXiv:2506.07665)
[6] Muttillo, V., Valente, G., Federici, F., Pomante, L., Faccio, M., Tieri, C., & Ferri, S. (2016). A design methodology for soft-core platforms on FPGA with SMP Linux, OpenMP support, and distributed hardware profiling system. EURASIP Journal on Embedded Systems, 2016, 15. https://doi.org/10.1186/s13639-016-0051-9
[7] US Patent Application 2016/0063164 A1 — Method and System for Hardware/Software Co-simulation and Processor Resource Allocation.
[8] US Patent 7,788,078 B1 — Processor/Memory Co-Exploration at Multiple Abstraction Levels (CoWare Inc. / Synopsys Inc.).
[9] US Patent 9,928,179 B2 — Cache Replacement Policy (Intel Corporation).
[10] US Patent 6,263,427 B1 — Branch Prediction Mechanism (Rise Technology Co.).
[11] gem5 Project. https://www.gem5.org
