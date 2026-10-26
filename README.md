# ⚡ Processor Design Studio

A full-stack processor exploration and cycle-accurate simulation platform for custom 32-bit RISC architectures. Featuring a C++17 OpenMP simulation core, Node.js REST API backend, and a modern Vite/React web dashboard.

---

## 🏛️ System Architecture

```
┌────────────────────────────────────────┐                     ┌────────────────────────────────────────┐
│            sim-core (C++17)            │  subprocess / CLI   │         backend (Node.js / Express)   │
│  • Functional & 5-Stage Pipelined CPU  │ <─────────────────> │  • REST API endpoints                  │
│  • Cache Controller & Branch Predictor │     JSON in/out     │  • ProcessBuilder executor             │
│  • OpenMP Parallel Sweep Runner        │                     └───────────────────┬────────────────────┘
└────────────────────────────────────────┘                                         │
                                                                                   │ REST / JSON
                                                                                   ▼
                                                               ┌────────────────────────────────────────┐
                                                               │         frontend (React + Vite)        │
                                                               │  • Config Builder                      │
                                                               │  • Pipeline Visualizer                 │
                                                               │  • Comparison Dashboard                │
                                                               └────────────────────────────────────────┘
```

---

## 🚀 Key Features

- **Toy RISC ISA**: 32-bit custom instruction set (19 instructions across R-type, I-type, J-type formats) with 16 registers ($r0–$r15).
- **Two-Pass Assembler**: Converts assembly text (`.asm`) with labels, comments, and immediate values into 32-bit machine code binaries.
- **5-Stage Pipeline**: Simulates `IF`, `ID`, `EX`, `MEM`, and `WB` cycles with configurable data forwarding paths and structural/RAW hazard stalling logic.
- **Configurable Cache Hierarchy**: Models L1 cache parameters:
  - Cache size: 1KB to 64KB
  - Block size: 16B to 128B
  - Associativity: Direct-mapped, 2-way, 4-way, 8-way
  - Replacement policies: `LRU`, `FIFO`, `Random`
  - Write policy: Write-through, no-write-allocate
- **Branch Prediction**: Supports `STATIC_NOT_TAKEN`, `STATIC_TAKEN`, `ONE_BIT`, and `TWO_BIT_SATURATING` branch predictors with dynamic flushing on misprediction.
- **OpenMP Parallel Sweeps**: Runs multi-parameter architectural sweeps concurrently across available CPU threads with linear speedup.
- **Interactive UI**: High-end React interface for custom CPU configuration building, cycle-by-cycle animated pipeline visualizer, and Recharts-inspired comparison charts.

---

## 📁 Repository Structure

```
processor-design-studio/
├── sim-core/                   # C++17 Simulator Core
│   ├── CMakeLists.txt
│   ├── include/                # Header files (ISA, pipeline, cache, predictor, sweep)
│   ├── src/                    # Implementation files
│   ├── benchmarks/             # Assembly benchmark suite (.asm)
│   └── tests/                  # Automated test suite (253 tests)
├── backend/                    # Node.js Express REST API server
│   ├── package.json
│   └── src/server.js
├── frontend/                   # React + Vite frontend application
│   ├── package.json
│   ├── index.html
│   └── src/
│       ├── pages/              # ConfigBuilder, PipelineVisualizer, Dashboard
│       └── App.jsx
├── docs/                       # Architectural specs and benchmark reports
│   └── REPORT.md
└── README.md
```

---

## 🛠️ Build & Run Instructions

### Prerequisites
- C++17 Compiler (GCC / MinGW / Clang / MSVC) with OpenMP support
- CMake (>= 3.16) & Ninja (or Make)
- Node.js (>= 16.x) & npm

### 1. Build C++ Simulator Core
From root directory (`Processor design studio`):
```powershell
cd sim-core
cmake -B build -G "Ninja"
cmake --build build --config Release
cd ..
```

### 2. Run Test Suite (253 Tests)
From root directory (`Processor design studio`):
```powershell
.\sim-core\build\test_all.exe
```

### 3. Start Backend REST API
From root directory (`Processor design studio`):
```powershell
cd backend
npm install
npm start
```
*Backend runs on `http://localhost:3001`.*

### 4. Start Frontend Development Server
In a **new terminal window**, from root directory (`Processor design studio`):
```powershell
cd frontend
npm install
npm run dev
```
*Frontend runs on `http://localhost:5173`.*


---

## 📊 Sample Execution Metrics

Sample parameter sweep results on `loop_sum` benchmark:

| Pipeline | Cache Config | Branch Predictor | Total Cycles | Instructions | CPI | IPC | Cache Hit Rate |
| :--- | :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **Functional** | None | STATIC_NOT_TAKEN | 44 | 44 | 1.000 | 1.000 | 0.0% |
| **Pipelined+Fwd** | 4KB 4-way LRU | TWO_BIT_SATURATING | 48 | 44 | 1.091 | 0.917 | 100.0% |
| **Pipelined+Stall** | 1KB Direct-Mapped | STATIC_TAKEN | 68 | 44 | 1.545 | 0.647 | 81.8% |

---

## 📜 License
MIT License. Built for Processor Design Studio architecture exploration.
