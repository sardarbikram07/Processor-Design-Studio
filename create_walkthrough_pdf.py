import os
import sys
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.pdfgen import canvas

PDF_FILENAME = "PROFESSOR_DEMO_WALKTHROUGH.pdf"

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica-Bold", 8)
        self.setFillColor(colors.HexColor("#0284c7"))
        
        # Header (pages > 1)
        if self._pageNumber > 1:
            self.drawString(36, 11 * inch - 26, "PROCESSOR DESIGN STUDIO")
            self.setFont("Helvetica", 8)
            self.setFillColor(colors.HexColor("#64748b"))
            self.drawString(165, 11 * inch - 26, "— Complete Demonstration Manual & Architecture Guide")
            self.setStrokeColor(colors.HexColor("#cbd5e1"))
            self.setLineWidth(0.5)
            self.line(36, 11 * inch - 30, 8.5 * inch - 36, 11 * inch - 30)

        # Footer (all pages)
        self.setFont("Helvetica", 7.5)
        self.setFillColor(colors.HexColor("#64748b"))
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(8.5 * inch - 36, 20, page_str)
        self.drawString(36, 20, "CONFIDENTIAL & PROPRIETARY — PREPARED FOR ACADEMIC EVALUATION & VIVA DEFENSE")
        self.setStrokeColor(colors.HexColor("#cbd5e1"))
        self.setLineWidth(0.5)
        self.line(36, 28, 8.5 * inch - 36, 28)
        
        self.restoreState()


def build_pdf():
    doc = SimpleDocTemplate(
        PDF_FILENAME,
        pagesize=letter,
        leftMargin=36,
        rightMargin=36,
        topMargin=36,
        bottomMargin=36
    )

    styles = getSampleStyleSheet()

    # Custom styles
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=20,
        leading=23,
        textColor=colors.HexColor('#0f172a'),
        spaceAfter=2
    )

    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=13,
        textColor=colors.HexColor('#0284c7'),
        spaceAfter=6
    )

    meta_style = ParagraphStyle(
        'DocMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.5,
        leading=10.5,
        textColor=colors.HexColor('#475569')
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=11.5,
        leading=14.5,
        textColor=colors.HexColor('#0f172a'),
        spaceBefore=10,
        spaceAfter=4,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=9.5,
        leading=12.5,
        textColor=colors.HexColor('#0369a1'),
        spaceBefore=7,
        spaceAfter=3,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.2,
        leading=11.2,
        textColor=colors.HexColor('#1e293b'),
        spaceAfter=4
    )

    bullet_style = ParagraphStyle(
        'Bullet_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor('#1e293b'),
        leftIndent=10,
        firstLineIndent=-6,
        spaceAfter=2
    )

    script_style = ParagraphStyle(
        'Script_Text',
        parent=styles['Normal'],
        fontName='Helvetica-Oblique',
        fontSize=8,
        leading=11,
        textColor=colors.HexColor('#14532d'),
        spaceBefore=2,
        spaceAfter=4
    )

    table_header = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.white,
        alignment=0
    )

    table_cell = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.HexColor('#1e293b')
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.HexColor('#0f172a')
    )

    story = []

    # =========================================================================
    # PAGE 1: SYSTEM OVERVIEW & WEBPAGE 1 (CONFIG BUILDER)
    # =========================================================================
    story.append(Paragraph("PROCESSOR DESIGN STUDIO", title_style))
    story.append(Paragraph("Complete Web Dashboard Manual, Feature Breakdown & Professor Demonstration Guide", subtitle_style))
    
    meta_text = (
        "<b>Domain:</b> Computer Architecture & Parallel Systems | "
        "<b>ISA:</b> 32-bit Custom RISC (19 Instructions, 16 Regs) | "
        "<b>Tech Stack:</b> C++17 (OpenMP), Node.js (Express), React (Vite) | "
        "<b>Live Ports:</b> Backend: <code>localhost:3001</code> · Frontend: <code>localhost:5173</code>"
    )
    story.append(Paragraph(meta_text, meta_style))
    story.append(Spacer(1, 4))
    story.append(HRFlowable(width="100%", thickness=1.2, color=colors.HexColor('#0284c7'), spaceAfter=8))

    story.append(Paragraph("1. System Architecture & Live Deployment Status", h1_style))
    story.append(Paragraph(
        "Processor Design Studio is a cycle-accurate hardware simulation and design-space exploration platform. "
        "It decouples the simulation kernel (C++17 for raw compute and OpenMP parallelism) from the backend API and React frontend.", body_style
    ))

    arch_table_data = [
        [Paragraph("Layer", table_header), Paragraph("Technology", table_header), Paragraph("Active Host Port / Path", table_header), Paragraph("Role & Execution Model", table_header)],
        [
            Paragraph("<b>Sim Core</b>", table_cell_bold),
            Paragraph("C++17, OpenMP, CMake", table_cell),
            Paragraph("sim-core/build/sim.exe", table_cell),
            Paragraph("Executes functional / 5-stage pipelined CPU simulation, L1 cache modeling, branch prediction, and multi-threaded parameter sweeps.", table_cell)
        ],
        [
            Paragraph("<b>Backend API</b>", table_cell_bold),
            Paragraph("Node.js, Express, child_process", table_cell),
            Paragraph("http://localhost:3001", table_cell),
            Paragraph("Thin wrapper invoking <code>sim.exe</code> via stdin/stdout JSON streaming. Exposes endpoints: <code>/health</code>, <code>/benchmarks</code>, <code>/simulate</code>, <code>/sweep</code>.", table_cell)
        ],
        [
            Paragraph("<b>Frontend UI</b>", table_cell_bold),
            Paragraph("React 18, Vite 5, Vanilla CSS", table_cell),
            Paragraph("http://localhost:5173", table_cell),
            Paragraph("Interactive three-tab web interface: Configuration Builder, Pipeline Visualizer, and Architectural Comparison Dashboard.", table_cell)
        ]
    ]
    t_arch = Table(arch_table_data, colWidths=[65, 95, 130, 250])
    t_arch.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#0f172a')),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#cbd5e1')),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor('#f8fafc')])
    ]))
    story.append(t_arch)
    story.append(Spacer(1, 6))

    story.append(Paragraph("2. Webpage Tab 1: Configuration Builder (http://localhost:5173)", h1_style))
    story.append(Paragraph(
        "<b>Purpose:</b> Allows custom microarchitecture configuration, single cycle-accurate simulation triggers, "
        "and real-time inspection of 7 execution telemetry metrics and raw configuration JSON vectors.", body_style
    ))

    config_features = [
        [Paragraph("Feature Component", table_header), Paragraph("How It Works Technically (C++ Core)", table_header), Paragraph("Architectural Purpose / Demonstration Goal", table_header)],
        [
            Paragraph("<b>Benchmark Workload Selection</b>", table_cell_bold),
            Paragraph("Queries <code>GET /benchmarks</code> to fetch available assembly files: <code>loop_sum</code>, <code>factorial</code>, <code>bubble_sort</code>, <code>array_sum</code>, <code>hazard_heavy</code>, <code>tight_loop</code>.", table_cell),
            Paragraph("Allows testing distinct workloads: memory-intensive, branch-intensive, recursive call stacks, and hazard-heavy sequences.", table_cell)
        ],
        [
            Paragraph("<b>5-Stage Pipelining Toggle</b>", table_cell_bold),
            Paragraph("When ON, initializes <code>PipelinedSimulator</code> modeling IF/ID/EX/MEM/WB with latch registers. When OFF, switches to single-cycle <code>Simulator</code> (CPI = 1.000).", table_cell),
            Paragraph("Demonstrates ideal single-cycle execution versus real-world pipelined instruction overlap and pipeline latency overheads.", table_cell)
        ],
        [
            Paragraph("<b>Data Forwarding Toggle</b>", table_cell_bold),
            Paragraph("Toggles EX/MEM & MEM/WB bypass multiplexers into ALU input operands. Detects RAW hazards: <code>(id_ex.rs == ex_mem.rd)</code> and forwards without stall.", table_cell),
            Paragraph("Demonstrates hardware forwarding paths resolving Read-After-Write hazards without inserting bubble stalls into the execution stage.", table_cell)
        ],
        [
            Paragraph("<b>Hazard Stalling Toggle</b>", table_cell_bold),
            Paragraph("Detects Load-Use hazards where <code>id_ex.opcode == LW</code> and freezes PC + IF/ID latches while injecting a NOP bubble into ID/EX latch.", table_cell),
            Paragraph("Shows mandatory pipeline bubbles when data cannot be forwarded in time (load-use dependency delay).", table_cell)
        ],
        [
            Paragraph("<b>L1 Data Cache Hierarchy</b>", table_cell_bold),
            Paragraph("Configurable Cache Size (0–128 KB, 0=disabled), Block Size (16–128 B), Associativity (1-way to 8-way), and Replacement (<code>LRU</code>, <code>FIFO</code>, <code>Random</code>).", table_cell),
            Paragraph("Demonstrates spatial and temporal locality. Shows how set associativity eliminates conflict misses on memory-dense algorithms.", table_cell)
        ],
        [
            Paragraph("<b>Branch Predictor Strategy</b>", table_cell_bold),
            Paragraph("Selects branch prediction strategy: <code>STATIC_NOT_TAKEN</code>, <code>STATIC_TAKEN</code>, <code>ONE_BIT</code>, or <code>TWO_BIT_SATURATING</code>. Tracks branch hits and flushes.", table_cell),
            Paragraph("Illustrates control hazard mitigation. Shows how 2-bit hysteresis counters prevent thrashing on loop termination branches.", table_cell)
        ]
    ]
    t_cfg = Table(config_features, colWidths=[120, 210, 210])
    t_cfg.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#0f172a')),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#cbd5e1')),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor('#f8fafc')])
    ]))
    story.append(t_cfg)
    story.append(Spacer(1, 5))

    story.append(Paragraph("<b>Telemetry Output Metrics Explained:</b>", h2_style))
    story.append(Paragraph("• <b>Total Cycles:</b> Elapsed clock cycles from first fetch to HALT retirement. &nbsp;|&nbsp; • <b>Instructions Executed:</b> Useful architectural instructions committed in WB.", bullet_style))
    story.append(Paragraph("• <b>CPI (Cycles Per Instruction):</b> Total Cycles / Instructions Executed. Pipelined approaches 1.0. &nbsp;|&nbsp; • <b>IPC:</b> 1 / CPI (throughput).", bullet_style))
    story.append(Paragraph("• <b>Stall Cycles:</b> Cycles frozen due to RAW hazards or Load-Use bubbles. &nbsp;|&nbsp; • <b>Cache Hit Rate:</b> % of memory accesses resolved in L1 Cache.", bullet_style))
    story.append(Paragraph("• <b>Branch Misprediction Rate:</b> % of conditional branches where speculative fetch was wrong, incurring a 2-cycle pipeline flush.", bullet_style))

    # =========================================================================
    # PAGE 2: WEBPAGE 2 (PIPELINE VISUALIZER) & WEBPAGE 3 (DASHBOARD)
    # =========================================================================
    story.append(PageBreak())
    story.append(Paragraph("3. Webpage Tab 2: Pipeline Stage Trace Visualizer", h1_style))
    story.append(Paragraph(
        "<b>Purpose:</b> Acts as an interactive logic analyzer. Provides cycle-by-cycle inspection of the 5 pipeline "
        "stage latches (IF, ID, EX, MEM, WB), visualizes instruction movement, and highlights data/control hazards.", body_style
    ))

    pipe_features = [
        [Paragraph("Pipeline Stage", table_header), Paragraph("Active Hardware Component", table_header), Paragraph("Function in Toy RISC Architecture", table_header)],
        [
            Paragraph("<b>IF (Fetch)</b>", table_cell_bold),
            Paragraph("Program Counter & Instruction Memory", table_cell),
            Paragraph("Fetches the 32-bit instruction word pointed to by PC from memory, increments PC by 4.", table_cell)
        ],
        [
            Paragraph("<b>ID (Decode)</b>", table_cell_bold),
            Paragraph("Instruction Decoder & Register File", table_cell),
            Paragraph("Splits 32-bit word into Opcode/Funct/Reg indices. Reads source registers <code>$rs</code> and <code>$rt</code> from Register File ($r0–$r15).", table_cell)
        ],
        [
            Paragraph("<b>EX (Execute)</b>", table_cell_bold),
            Paragraph("ALU & Branch Resolution Unit", table_cell),
            Paragraph("Performs arithmetic/logical operation or evaluates branch conditions (BEQ, BNE, BLT, BGE). Target address calculated.", table_cell)
        ],
        [
            Paragraph("<b>MEM (Memory)</b>", table_cell_bold),
            Paragraph("L1 Data Cache Controller", table_cell),
            Paragraph("Performs word-aligned memory access for LW (read) and SW (write) against L1 Cache / flat memory.", table_cell)
        ],
        [
            Paragraph("<b>WB (Write Back)</b>", table_cell_bold),
            Paragraph("Register File Write Port", table_cell),
            Paragraph("Commits ALU result or loaded memory value into destination register <code>$rd</code> or <code>$rt</code> ($r0 writes ignored).", table_cell)
        ]
    ]
    t_pipe = Table(pipe_features, colWidths=[90, 180, 270])
    t_pipe.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#0f172a')),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#cbd5e1')),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor('#f8fafc')])
    ]))
    story.append(t_pipe)
    story.append(Spacer(1, 4))

    story.append(Paragraph("<b>Visualizer Controls & Hazard Highlights:</b>", h2_style))
    story.append(Paragraph("• <b>Cycle Step Buttons (&lt; PREV / NEXT &gt;):</b> Step forward/backward cycle-by-cycle to trace instruction flow across stages.", bullet_style))
    story.append(Paragraph("• <b>Hazard Badges:</b> Highlights DATA HAZARDS (RAW) in Amber, CONTROL HAZARDS (Branches) in Magenta, and STALL BUBBLES in Cyan.", bullet_style))
    story.append(Paragraph("• <b>Bypass Telemetry:</b> Badges confirm active <code>EX/MEM -&gt; EX</code> and <code>MEM/WB -&gt; EX</code> hardware forwarding lines.", bullet_style))
    story.append(Spacer(1, 8))

    story.append(Paragraph("4. Webpage Tab 3: Comparison & Architectural Sweep Dashboard", h1_style))
    story.append(Paragraph(
        "<b>Purpose:</b> Executes multi-dimensional parameter sweeps across architectural combinations, aggregates results "
        "via OpenMP multithreading, and generates live comparative charts and a searchable data matrix.", body_style
    ))

    dash_features = [
        [Paragraph("Dashboard Component", table_header), Paragraph("How It Works Technically", table_header), Paragraph("Analytical Purpose & Value", table_header)],
        [
            Paragraph("<b>OpenMP Parallel Sweep Runner</b>", table_cell_bold),
            Paragraph("Sends <code>{ parallel: true }</code> to backend. C++ core executes <code>#pragma omp parallel for schedule(dynamic,1)</code> across CPU cores.", table_cell),
            Paragraph("Accelerates architectural exploration by running 150+ configurations concurrently, capturing speedup and Amdahl's Law serial fraction.", table_cell)
        ],
        [
            Paragraph("<b>Telemetry Summary Strip</b>", table_cell_bold),
            Paragraph("Computes real-time statistics: Total Configs Evaluated, Optimum (Minimum) CPI, Maximum CPI, and Cross-Config Mean CPI.", table_cell),
            Paragraph("Gives instant high-level visibility into best-performing and worst-performing microarchitectural designs.", table_cell)
        ],
        [
            Paragraph("<b>Dynamic Bar Charts</b>", table_cell_bold),
            Paragraph("Renders live comparative charts with selectable metric axis: CPI, IPC, Stall Cycles, Misprediction Rate, Cache Hit Rate, or Wall Time.", table_cell),
            Paragraph("Visually correlates microarchitectural features (e.g. branch predictor algorithm vs misprediction rate).", table_cell)
        ],
        [
            Paragraph("<b>Interactive Data Matrix</b>", table_cell_bold),
            Paragraph("Sortable HTML table with column filters for Workload and Predictor. Displays CPI, IPC, Stalls, Hit %, Mispred %, and runtime.", table_cell),
            Paragraph("Allows in-depth numerical inspection of any individual configuration tested during the sweep.", table_cell)
        ]
    ]
    t_dash = Table(dash_features, colWidths=[120, 210, 210])
    t_dash.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor('#0f172a')),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor('#cbd5e1')),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor('#f8fafc')])
    ]))
    story.append(t_dash)

    # =========================================================================
    # PAGE 3: 5 STEP-BY-STEP PROFESSOR DEMONSTRATIONS
    # =========================================================================
    story.append(PageBreak())
    story.append(Paragraph("5. Step-by-Step Professor Demonstration Guide (5 Key Experiments)", h1_style))
    story.append(Paragraph(
        "Follow these exact steps during your demonstration to showcase the full depth of your microarchitecture platform. "
        "Each test case includes exact UI clicks and a word-for-word explanation script.", body_style
    ))

    # Test Case 1
    story.append(Paragraph("Test Case 1: Data Forwarding vs. RAW Hazard Stalling (Pipeline Unit)", h2_style))
    story.append(Paragraph("<b>Steps on UI:</b> In <b>Config Builder</b>, select Benchmark: <code>hazard_heavy.asm</code>, turn Pipeline <b>ON</b>.<br/>"
                           "• <b>Exp A (No Forwarding):</b> Uncheck 'Data Forwarding', leave 'Hazard Stalling' checked. Click <b>RUN SIMULATION</b>. Observe CPI: <b>~2.384</b> (Stalls: 36–41).<br/>"
                           "• <b>Exp B (With Forwarding):</b> Check 'Data Forwarding' <b>ON</b>. Click <b>RUN SIMULATION</b>. CPI drops to <b>~1.307</b> (Stalls drop to 4–10).<br/>"
                           "• Click <b>'INSPECT PIPELINE TRACE'</b> to show EX/MEM bypass in action.", body_style))
    story.append(Paragraph("<b>What to Say:</b> <i>'Professor, here we observe our hazard detection and forwarding units. Without forwarding, consecutive Read-After-Write dependencies force the hazard unit to stall the pipeline, inserting bubbles into the EX stage and inflating CPI to 2.384. When forwarding is enabled, bypass paths route results directly from the EX/MEM and MEM/WB latches back to ALU inputs, boosting throughput by over 45%.'</i>", script_style))

    # Test Case 2
    story.append(Paragraph("Test Case 2: Cache Associativity & Eviction Policies (Memory Unit)", h2_style))
    story.append(Paragraph("<b>Steps on UI:</b> In <b>Config Builder</b>, select Benchmark: <code>array_sum.asm</code> or <code>bubble_sort.asm</code>.<br/>"
                           "• <b>Exp A (Direct-Mapped):</b> Cache Size: 1 KB, Block Size: 16 B, Associativity: <b>Direct Mapped</b>. Click <b>RUN SIMULATION</b>. Hit Rate: <b>~75.0%</b>.<br/>"
                           "• <b>Exp B (4-Way Associative LRU):</b> Cache Size: 4 KB, Block Size: 64 B, Associativity: <b>4-Way</b>, Policy: <b>LRU</b>. Click <b>RUN SIMULATION</b>. Hit Rate increases to <b>&gt;90% - 93%</b>.", body_style))
    story.append(Paragraph("<b>What to Say:</b> <i>'With a 1KB direct-mapped cache, memory accesses suffer conflict misses whenever active array addresses hash to the same set index. Expanding to a 4KB 4-way set-associative cache with Least Recently Used (LRU) eviction eliminates conflict misses and preserves spatial block locality, driving hit rates above 90%.'</i>", script_style))

    # Test Case 3
    story.append(Paragraph("Test Case 3: Branch Predictor Sophistication (Control Flow Unit)", h2_style))
    story.append(Paragraph("<b>Steps on UI:</b> In <b>Config Builder</b>, select Benchmark: <code>loop_sum.asm</code>.<br/>"
                           "• <b>Exp A:</b> Set Predictor to <code>STATIC_TAKEN</code>. Click <b>RUN SIMULATION</b>. Misprediction Rate: <b>~90.9%</b> (10 flushes out of 11 branches).<br/>"
                           "• <b>Exp B:</b> Set Predictor to <code>TWO_BIT_SATURATING</code>. Click <b>RUN SIMULATION</b>. Misprediction Rate drops dramatically to <b>~9.09%</b> (only 1 flush on final loop exit).", body_style))
    story.append(Paragraph("<b>What to Say:</b> <i>'In loop structures, exit branches evaluate to False during iterations. Static Taken prediction incurs a 2-stage pipeline flush penalty on every iteration. Our 2-bit saturating counter predictor uses a 4-state hysteresis state machine (Strongly/Weakly Taken/Not-Taken) which absorbs iterations and achieves 90.9% accuracy.'</i>", script_style))

    # Test Case 4
    story.append(Paragraph("Test Case 4: Functional Single-Cycle Baseline vs. Cycle-Accurate Pipelining", h2_style))
    story.append(Paragraph("<b>Steps on UI:</b> In <b>Config Builder</b>, select Benchmark: <code>factorial.asm</code>.<br/>"
                           "• <b>Exp A:</b> Turn 5-Stage Pipelining <b>OFF</b>. Run simulation. Observe: <b>Total Cycles == Instructions Executed (Ideal CPI = 1.000)</b>.<br/>"
                           "• <b>Exp B:</b> Turn 5-Stage Pipelining <b>ON</b>. Run simulation. Observe: Total Cycles includes pipeline fill latency and subroutine call delays.", body_style))
    story.append(Paragraph("<b>What to Say:</b> <i>'This validates architectural fidelity. In functional mode, each instruction commits in a single cycle (ideal CPI 1.0). In pipelined mode, our engine models the real hardware cost: pipeline fill cycles, subroutine jumps (JAL/JR), and return address latching.'</i>", script_style))

    # Test Case 5
    story.append(Paragraph("Test Case 5: OpenMP Parallel Exploration & Amdahl's Law Speedup", h2_style))
    story.append(Paragraph("<b>Steps on UI:</b> Navigate to <b>Comparison Dashboard</b> tab. Toggle <b>OPENMP PARALLEL</b> mode. Click <b>RUN FULL PARAMETER SWEEP</b>. Watch live execution across 150+ configurations.", body_style))
    story.append(Paragraph("<b>What to Say:</b> <i>'To evaluate hundreds of design combinations, we parallelized the C++ simulation core using OpenMP (#pragma omp parallel for). As documented in our benchmark report, we measured an empirical speedup of 1.48x on 2 cores and derived the non-parallel serial fraction (s ≈ 35.1%) due to JSON parsing and benchmark file I/O per Amdahl’s Law.'</i>", script_style))

    # =========================================================================
    # PAGE 4: VIVA DEFENSE & FAQ
    # =========================================================================
    story.append(PageBreak())
    story.append(Paragraph("6. Professor Viva Defense: Frequently Asked Questions & Answers", h1_style))
    story.append(Paragraph(
        "Be prepared for these standard questions that professors and evaluators frequently ask during project defense:", body_style
    ))

    qa_list = [
        ("Q1: How does your tool compare to gem5 or SimpleScalar?",
         "gem5 is an industry-standard full-system simulator for production ISAs (x86, ARM, RISC-V) running millions of instructions with OS support, "
         "but it is 10^5x slower than silicon and lacks interactive visual pipeline inspectors. Our Processor Design Studio focuses on rapid microarchitectural "
         "design-space exploration (DSE) of fundamental textbook concepts (Hennessy & Patterson) using a custom 19-instruction toy RISC ISA and native OpenMP multi-core parallel sweeps."),
        
        ("Q2: Why is register $r0 hardwired to zero?",
         "Hardwiring $r0 to 0 simplifies the ISA. It allows synthesis of NOPs (ADD $r0, $r0, $r0), register clearing, unconditional register copying (ADD $rd, $rs, $r0), "
         "and absolute addressing offsets without requiring dedicated ALU instructions."),
        
        ("Q3: Why can't all RAW hazards be eliminated by data forwarding?",
         "Load-Use dependencies cannot be completely forwarded because loaded data is not available until the end of the MEM stage, whereas the dependent instruction "
         "needs the operand at the beginning of the EX stage. This requires a mandatory 1-cycle stall bubble inserted by the hazard unit."),
        
        ("Q4: What is the write policy of your cache?",
         "Our L1 Data Cache implements Write-Through with No-Write-Allocate. On a write hit, both the cache line and backing main memory are updated. "
         "On a write miss, the write updates memory directly without fetching the block into cache, avoiding cache pollution on single-use writes."),
        
        ("Q5: Why did OpenMP speedup plateau at higher thread counts in your tests?",
         "Because each individual benchmark execution takes less than 1 millisecond, the serial overhead (parsing stdin JSON, loading .asm files from disk, and stringifying JSON results) "
         "accounts for approximately 35% of the total wall time. According to Amdahl’s Law, with a serial fraction s ≈ 35%, maximum theoretical speedup is capped at 1/s ≈ 2.85x.")
    ]

    for q, a in qa_list:
        story.append(Paragraph(f"<b>{q}</b>", h2_style))
        story.append(Paragraph(a, body_style))
        story.append(Spacer(1, 3))

    story.append(Spacer(1, 6))
    story.append(HRFlowable(width="100%", thickness=1, color=colors.HexColor('#cbd5e1'), spaceAfter=6))
    story.append(Paragraph("<b>Automated Verification Summary:</b> 253 / 253 automated unit and integration tests passing in <code>sim-core/build/test_all.exe</code> across 16 microarchitectural groups.", ParagraphStyle('FooterNote', parent=styles['Normal'], fontName='Helvetica-Bold', fontSize=8, textColor=colors.HexColor('#0284c7'))))

    # Build the document
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"Successfully generated clean {PDF_FILENAME}")

if __name__ == "__main__":
    build_pdf()
