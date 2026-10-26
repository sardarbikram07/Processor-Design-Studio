import { useState } from 'react';
import { API } from '../App';
import { 
  IconPipeline, 
  IconTerminal, 
  IconArrowRight,
  IconMetrics
} from '../components/Icons';

const STAGES = ['IF', 'ID', 'EX', 'MEM', 'WB'];
const STAGE_COLORS = {
  IF:  'var(--neon-cyan)',
  ID:  'var(--neon-green)',
  EX:  'var(--neon-amber)',
  MEM: 'var(--neon-violet)',
  WB:  'var(--neon-magenta)',
};

const HAZARD_TYPES = {
  data:    { color: 'var(--neon-amber)',   label: 'DATA HAZARD (RAW)',      bg: 'rgba(255,204,0,0.1)' },
  control: { color: 'var(--neon-magenta)', label: 'CONTROL HAZARD (BRANCH)', bg: 'rgba(255,0,85,0.1)' },
  stall:   { color: 'var(--neon-cyan)',    label: 'PIPELINE STALL BUBBLE',  bg: 'rgba(0,240,255,0.1)' },
};

// Simulate a static pipeline trace for the loaded result
function buildTrace(benchmark) {
  const traces = {
    loop_sum: [
      { cycle: 1,  stages: { IF: 'ADDI $r1,10', ID: 'NOP',       EX: 'NOP',       MEM: 'NOP',      WB: 'NOP' } },
      { cycle: 2,  stages: { IF: 'ADDI $r2,0',  ID: 'ADDI $r1',  EX: 'NOP',       MEM: 'NOP',      WB: 'NOP' } },
      { cycle: 3,  stages: { IF: 'BEQ $r1,$r0', ID: 'ADDI $r2',  EX: 'ADDI $r1',  MEM: 'NOP',      WB: 'NOP' } },
      { cycle: 4,  stages: { IF: 'ADD $r2,$r1', ID: 'BEQ $r1',   EX: 'ADDI $r2',  MEM: 'ADDI $r1', WB: 'NOP' } },
      { cycle: 5,  stages: { IF: 'ADDI $r1,-1', ID: 'ADD $r2',   EX: '[STALL]',   MEM: 'ADDI $r2', WB: 'ADDI $r1' }, hazard: 'data' },
      { cycle: 6,  stages: { IF: 'J loop',      ID: 'ADDI $r1',  EX: 'ADD $r2',   MEM: 'BEQ',      WB: 'ADDI $r2' } },
      { cycle: 7,  stages: { IF: '[FLUSH]',     ID: '[FLUSH]',   EX: 'ADDI $r1',  MEM: 'ADD $r2',  WB: 'BEQ'  }, hazard: 'control' },
      { cycle: 8,  stages: { IF: 'BEQ $r1,$r0', ID: '[BUBBLE]',  EX: '[BUBBLE]',  MEM: 'ADDI $r1', WB: 'ADD' } },
    ],
    default: [
      { cycle: 1,  stages: { IF: 'INSTR_1', ID: 'NOP',     EX: 'NOP',     MEM: 'NOP',    WB: 'NOP' } },
      { cycle: 2,  stages: { IF: 'INSTR_2', ID: 'INSTR_1', EX: 'NOP',     MEM: 'NOP',    WB: 'NOP' } },
      { cycle: 3,  stages: { IF: 'INSTR_3', ID: 'INSTR_2', EX: 'INSTR_1', MEM: 'NOP',    WB: 'NOP' } },
      { cycle: 4,  stages: { IF: 'INSTR_4', ID: 'INSTR_3', EX: 'INSTR_2', MEM: 'INSTR_1', WB: 'NOP' } },
      { cycle: 5,  stages: { IF: 'INSTR_5', ID: 'INSTR_4', EX: 'INSTR_3', MEM: 'INSTR_2', WB: 'INSTR_1' } },
    ],
  };
  return traces[benchmark] || traces.default;
}

export default function PipelineVisualizer({ lastResult }) {
  const [benchmark, setBenchmark] = useState('loop_sum');
  const [showForwarding, setShowForwarding] = useState(true);
  const [currentCycle, setCurrentCycle] = useState(0);

  const trace = buildTrace(benchmark);
  const totalCycles = trace.length;

  function prev() { setCurrentCycle(c => Math.max(0, c - 1)); }
  function next() { setCurrentCycle(c => Math.min(totalCycles - 1, c + 1)); }

  const row = trace[currentCycle];
  const hazard = row.hazard ? HAZARD_TYPES[row.hazard] : null;

  return (
    <div className="fade-in">
      <div className="section-header">
        <div className="section-icon">
          <IconPipeline size={22} color="var(--neon-cyan)" />
        </div>
        <div>
          <h1><span className="gradient-text">Pipeline Stage Trace Visualizer</span></h1>
          <p style={{ color: 'var(--text-sec)', fontSize: '0.85rem', fontFamily: 'var(--font-mono)' }}>
            CYCLE_ACCURATE // Step-by-step logic analyzer execution trace
          </p>
        </div>
      </div>

      {/* Control Bar */}
      <div className="card" style={{ marginBottom: '1rem' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flexWrap: 'wrap' }}>
          <div className="form-group" style={{ flex: 1, minWidth: 180 }}>
            <label className="form-label">
              <span>Workload Target</span>
              <span style={{ color: 'var(--neon-cyan)' }}>TRACE_LOG</span>
            </label>
            <select className="select" value={benchmark}
              onChange={e => { setBenchmark(e.target.value); setCurrentCycle(0); }}>
              {['loop_sum', 'factorial', 'bubble_sort', 'hazard_heavy'].map(b =>
                <option key={b} value={b}>{b}.asm</option>)}
            </select>
          </div>
          <div style={{ flex: 1, minWidth: 200 }}>
            <div className="form-label" style={{ marginBottom: 6 }}>Bypass Paths (Forwarding)</div>
            <div style={{ display: 'flex', gap: 6 }}>
              <button className={`btn ${showForwarding ? 'btn-primary' : 'btn-secondary'}`}
                onClick={() => setShowForwarding(true)}>ENABLED</button>
              <button className={`btn ${!showForwarding ? 'btn-primary' : 'btn-secondary'}`}
                onClick={() => setShowForwarding(false)}>DISABLED</button>
            </div>
          </div>
          {lastResult && (
            <div style={{ display: 'flex', gap: '0.5rem', marginLeft: 'auto', flexWrap: 'wrap' }}>
              <span className="badge badge-cyan">CPI: {lastResult.cpi?.toFixed(3)}</span>
              <span className="badge badge-green">IPC: {lastResult.ipc?.toFixed(3)}</span>
              <span className="badge badge-amber">STALLS: {lastResult.stallCycles}</span>
            </div>
          )}
        </div>
      </div>

      {/* Pipeline Stage Display */}
      <div className="card card-cyan" style={{ marginBottom: '1rem' }}>
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '1.25rem' }}>
          <h3 style={{ fontFamily: 'var(--font-mono)' }}>
            <span style={{ color: 'var(--neon-cyan)' }}>[CYCLE {currentCycle + 1} / {totalCycles}]</span> STAGE_LATCHES
          </h3>
          {hazard && (
            <div style={{
              display: 'flex', alignItems: 'center', gap: 8,
              padding: '0.35rem 0.85rem',
              background: hazard.bg,
              border: `1px solid ${hazard.color}`,
              fontFamily: 'var(--font-mono)', fontSize: '0.72rem', fontWeight: 700,
              color: hazard.color,
            }}>
              <span style={{ width: 6, height: 6, background: hazard.color }} />
              <span>{hazard.label}</span>
            </div>
          )}
        </div>

        <div className="pipeline-stages">
          {STAGES.map(stage => {
            const instr = row.stages[stage] || 'NOP';
            const isBubble = instr === 'NOP' || instr.includes('BUBBLE') || instr.includes('FLUSH');
            const isStall  = instr.includes('STALL');
            const isActive = !isBubble;

            return (
              <div key={stage} className={`stage ${isActive ? 'active' : ''}`}>
                <div className="stage-name" style={{ color: STAGE_COLORS[stage] }}>
                  {stage}
                </div>
                <div className="stage-instr" style={{
                  color: isStall ? 'var(--neon-amber)' :
                         isBubble ? 'var(--text-muted)' :
                         instr.includes('FLUSH') ? 'var(--neon-magenta)' : 'var(--neon-cyan)',
                }}>
                  {instr}
                </div>
              </div>
            );
          })}
        </div>

        {/* Forwarding paths telemetry */}
        {showForwarding && (
          <div style={{ marginTop: '0.75rem', display: 'flex', gap: 8, flexWrap: 'wrap' }}>
            {hazard?.label?.includes('DATA') && (
              <>
                <span className="badge badge-amber">EX/MEM -&gt; EX BYPASS ACTIVE</span>
                <span className="badge badge-cyan">MEM/WB -&gt; EX BYPASS ACTIVE</span>
              </>
            )}
            {!hazard && (
              <span className="badge badge-green">NO HAZARD DETECTED IN THIS CYCLE</span>
            )}
          </div>
        )}
      </div>

      {/* Cycle Navigation */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '1rem', justifyContent: 'center', margin: '1.5rem 0' }}>
        <button className="btn btn-secondary" onClick={prev} disabled={currentCycle === 0}>
          &lt; PREV CYCLE
        </button>
        <div style={{ display: 'flex', gap: 5 }}>
          {trace.map((_, i) => (
            <div key={i}
              onClick={() => setCurrentCycle(i)}
              style={{
                width: 12, height: 12, cursor: 'pointer',
                border: i === currentCycle ? '1px solid var(--neon-cyan)' : '1px solid var(--border-tech)',
                background: i === currentCycle ? 'var(--neon-cyan)' :
                            trace[i].hazard ? (HAZARD_TYPES[trace[i].hazard]?.color || 'var(--border-tech)') : 'var(--bg-input)',
                transition: 'all 0.15s ease',
              }} />
          ))}
        </div>
        <button className="btn btn-secondary" onClick={next} disabled={currentCycle === totalCycles - 1}>
          NEXT CYCLE &gt;
        </button>
      </div>

      {/* Stage Specification Reference */}
      <div className="card">
        <h3><IconTerminal size={16} color="var(--neon-cyan)" /> Stage Function Reference</h3>
        <div className="grid-3" style={{ gap: '0.85rem', marginTop: '1rem' }}>
          {[
            { stage: 'IF',  title: 'Instruction Fetch',  desc: 'Fetch 32-bit instruction word from Instruction Memory at PC' },
            { stage: 'ID',  title: 'Instruction Decode', desc: 'Decode Opcode & Funct, read Register File ($r0-$r15)' },
            { stage: 'EX',  title: 'Execute / ALU',       desc: 'ALU calculation, branch condition target evaluation' },
            { stage: 'MEM', title: 'Memory Access',      desc: 'Execute LW / SW memory access against L1 Data Cache' },
            { stage: 'WB',  title: 'Write Back',         desc: 'Commit result word into destination register in RegFile' },
          ].map(s => (
            <div key={s.stage} style={{
              padding: '0.85rem',
              background: 'var(--bg-input)',
              border: '1px solid var(--border-tech)',
            }}>
              <div style={{
                fontFamily: 'var(--font-mono)', fontWeight: 700, fontSize: '0.8rem',
                color: STAGE_COLORS[s.stage], marginBottom: '0.35rem',
              }}>{s.stage} // {s.title}</div>
              <div style={{ fontSize: '0.72rem', color: 'var(--text-sec)', fontFamily: 'var(--font-mono)' }}>{s.desc}</div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
