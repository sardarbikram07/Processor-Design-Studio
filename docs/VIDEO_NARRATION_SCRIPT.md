# Processor Design Studio — Video Demonstration & Narration Script

> **Project:** Processor Design Studio — Interactive RISC Microarchitecture Simulation & Exploration Platform  
> **Author:** Bikram Sardar | **Registration No:** 26MAI0065 | **Institution:** VIT Vellore  
> **Target Media:** Screen Recording Video ([`images/processor_studio_full_demo.webp`](../images/processor_studio_full_demo.webp) / [`images/demo_walkthrough.webp`](../images/demo_walkthrough.webp))  
> **Duration:** ~2.5 to 3 minutes

---

## 🎬 Video Recording Files Location
- **Primary Full Recording:** [`images/processor_studio_full_demo.webp`](file:///d:/Desktop/Processor%20design%20studio/images/processor_studio_full_demo.webp) *(7.3 MB, complete interactive multi-page walkthrough)*
- **Concise 3-Test Case Walkthrough:** [`images/demo_walkthrough.webp`](file:///d:/Desktop/Processor%20design%20studio/images/demo_walkthrough.webp)

---

## 🎙️ Complete Synchronized Narration Script

### [00:00 - 00:25] Segment 1: Introduction & Architecture Overview
* **On Screen:** Browser opens `http://localhost:5173`. The Processor Design Studio landing page and Config Builder load with modern dark theme and parameter cards.
* **Speaker (Voiceover):**
> *"Hello respected evaluators. Welcome to the demonstration of **Processor Design Studio** — an interactive, cycle-accurate RISC microarchitecture design space exploration and visualization platform.*
>
> *Modern silicon prototyping faces soaring costs and lengthy simulation cycles. Our system addresses this by providing an end-to-end framework featuring a high-performance C++17 simulation core, multi-threaded OpenMP sweep parallelization, and a modern web dashboard for real-time microarchitectural telemetry."*

---

### [00:25 - 01:15] Segment 2: Test Case 1 — Config Builder & Simulation Execution
* **On Screen:** The presenter selects `matrix_multiply.asm` from the benchmark dropdown, confirms 5-Stage Pipelining with Forwarding and Stalling enabled, sets Cache to 4KB (4-Way Associative, LRU replacement), configures the Branch Predictor to `TWO_BIT_SATURATING`, and clicks **Run Simulation**. Live telemetry cards appear.
* **Speaker (Voiceover):**
> *"In our first test case, we navigate the **Config Builder** to define a custom processor microarchitecture.*
>
> *Here, we select the computation-intensive `matrix_multiply` benchmark. For our execution pipeline, we enable 5-stage pipelining with internal ALU data forwarding and dynamic hazard detection.*
>
> *For the memory subsystem, we configure a 4 Kilobyte L1 data cache with 4-way set associativity and Least Recently Used (LRU) replacement policy.*
>
> *For branch management, we choose a 2-Bit Saturating Counter predictor. Upon clicking **Run Simulation**, the backend dispatches our configuration into the native C++ engine.*
>
> *Notice the instantaneous real-time telemetry: the processor completes the workload in 1,039 clock cycles with an exceptional CPI of **1.208** and an IPC of **0.828**. Because data forwarding is active, data hazard stall cycles are minimized to zero, achieving maximum execution throughput."*

---

### [01:15 - 02:05] Segment 3: Test Case 2 — Cycle-by-Cycle Pipeline Visualizer
* **On Screen:** The presenter clicks the **Pipeline Visualizer** navigation tab. The 5-stage pipeline canvas appears showing `IF`, `ID`, `EX`, `MEM`, and `WB` latches. The user clicks **Next Cycle** repeatedly from Cycle 1 to Cycle 5.
* **Speaker (Voiceover):**
> *"Next, in our second test case, we transition to the **Pipeline Visualizer** to examine instruction flow at cycle-level granularity.*
>
> *This interface visualizes the five classical pipeline stages: Instruction Fetch, Instruction Decode, Execute, Memory Access, and Write-Back, alongside internal pipeline register latches.*
>
> *As we step clock-by-clock through execution cycles:
> - In Cycle 1 and 2, instructions are fetched and decoded into the register file.
> - By Cycle 3 and 4, instructions advance into the arithmetic and memory execution units.
> - When a Read-After-Write (RAW) data dependency occurs, the hardware hazard detection unit transparently resolves it. Rather than stalling execution, data bypass multiplexers forward operands directly from the EX/MEM and MEM/WB latches straight into the ALU inputs.*
>
> *This real-time visual feedback proves the correctness of our latch snapshot engine and demonstrates hazard mitigation in action."*

---

### [02:05 - 02:45] Segment 4: Test Case 3 — Comparison Dashboard & Parallel Sweeps
* **On Screen:** The presenter clicks the **Comparison Dashboard** tab, scrolling through comparative charts, cache replacement policy bars, branch predictor misprediction curves, and OpenMP thread scaling analysis.
* **Speaker (Voiceover):**
> *"In our third test case, we explore the **Comparison Dashboard**, designed for multi-parameter Design Space Exploration.*
>
> *The dashboard aggregates empirical metrics across the architectural search space:
> - **Cache Subsystem Analysis**: 4-way LRU achieves a 93.1% hit rate on memory-bound workloads, outperforming FIFO at 90.3% and Random replacement at 85.9%.
> - **Branch Prediction**: The 2-Bit Saturating FSM reduces misprediction penalties down to just 9.09% on loops, demonstrating state hysteresis.
> - **Parallel Thread Scaling**: Utilizing OpenMP multi-threading (`#pragma omp parallel for`), parallel sweeps achieve a **1.48× empirical speedup** across CPU cores, strictly adhering to Amdahl’s Law with a derived non-parallel serial fraction $s \approx 35.1\%$.*
>
> *This allows computer architects to rapidly identify Pareto-optimal configurations before tape-out."*

---

### [02:45 - 03:00] Segment 5: Conclusion & Verification Summary
* **On Screen:** Presenter scrolls back to top overview or displays test summary.
* **Speaker (Voiceover):**
> *"To guarantee rigorous reliability, every component of this platform is verified against **124 automated test cases executing 253 individual assertions** with a **100% pass rate**.*
>
> *In summary, Processor Design Studio combines cycle-accurate RISC simulation, parallel design space exploration, and interactive visualization into a unified, research-grade tool. Thank you!"*

---

## 💡 Quick Tips for Presentation / Voice Recording
1. **Pacing:** Speak in a calm, confident, and professional academic tone.
2. **Audio Sync:** If recording live with microphone, play the video in a browser or media player and speak following the timestamp milestones.
3. **Key Highlight Words:** Emphasize *"Cycle-accurate"*, *"Data forwarding"*, *"4-way LRU"*, *"Amdahl's Law"*, and *"100% verification pass rate"*.
