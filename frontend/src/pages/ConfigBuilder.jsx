import { useState, useEffect } from 'react';
import { API } from '../App';
import { 
  IconConfig, 
  IconTerminal, 
  IconPipeline, 
  IconMemory, 
  IconPredictor, 
  IconPlay, 
  IconMetrics,
  IconArrowRight
} from '../components/Icons';

const BENCHMARKS = ['loop_sum', 'factorial', 'bubble_sort', 'array_sum', 'hazard_heavy', 'tight_loop'];
const PREDICTORS = ['STATIC_NOT_TAKEN', 'STATIC_TAKEN', 'ONE_BIT', 'TWO_BIT_SATURATING'];
const REPLACEMENTS = ['LRU', 'FIFO', 'Random'];

function Toggle({ checked, onChange, label }) {
  return (
    <div className="toggle-row">
      <span className="toggle-label">{label}</span>
      <div className={`toggle ${checked ? 'on' : ''}`} onClick={() => onChange(!checked)} />
    </div>
  );
}

function MetricCard({ label, value, sub, accentClass }) {
  return (
    <div className={`metric ${accentClass || ''}`}>
      <span className="metric-label">{label}</span>
      <span className="metric-value">{value}</span>
      {sub && <span className="metric-sub">{sub}</span>}
    </div>
  );
}

export default function ConfigBuilder({ onResult, onGoTo }) {
  const [config, setConfig] = useState({
    benchmark: 'loop_sum',
    pipeline: { enabled: true, forwarding: true, stalling: true },
    cache: { sizeKB: 0, blockSize: 64, associativity: 1, replacement: 'LRU', writePolicy: 'write-through' },
    branchPredictor: { type: 'STATIC_NOT_TAKEN' },
  });
  const [result, setResult]     = useState(null);
  const [loading, setLoading]   = useState(false);
  const [error, setError]       = useState('');
  const [benchmarks, setBenchmarks] = useState(BENCHMARKS);

  useEffect(() => {
    fetch(`${API}/benchmarks`)
      .then(r => r.json())
      .then(d => { if (d.benchmarks?.length) setBenchmarks(d.benchmarks); })
      .catch(() => {});
  }, []);

  const set = (path, val) => {
    const parts = path.split('.');
    setConfig(prev => {
      const next = structuredClone(prev);
      let obj = next;
      for (let i = 0; i < parts.length - 1; i++) obj = obj[parts[i]];
      obj[parts[parts.length - 1]] = val;
      return next;
    });
  };

  async function run() {
    setLoading(true); setError(''); setResult(null);
    try {
      const res = await fetch(`${API}/simulate`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config),
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Simulation failed');
      setResult(data);
      onResult?.(data);
    } catch (e) {
      setError(e.message);
    } finally {
      setLoading(false);
    }
  }

  const cpi     = result?.cpi?.toFixed(3) || '—';
  const ipc     = result?.ipc?.toFixed(3) || '—';
  const stalls  = result?.stallCycles ?? '—';
  const hitRate = result?.cacheHitRate != null ? (result.cacheHitRate * 100).toFixed(1) + '%' : '—';
  const mispred = result?.branchMispredictionRate != null ? (result.branchMispredictionRate * 100).toFixed(1) + '%' : '—';

  return (
    <div className="fade-in">
      <div className="section-header">
        <div className="section-icon">
          <IconConfig size={22} color="var(--neon-cyan)" />
        </div>
        <div>
          <h1><span className="gradient-text">Configuration Builder</span></h1>
          <p style={{ color: 'var(--text-sec)', fontSize: '0.85rem', fontFamily: 'var(--font-mono)' }}>
            SYS_CONFIG // Set microarchitectural parameters and execute simulation
          </p>
        </div>
      </div>

      <div className="grid-2" style={{ gap: '1.5rem' }}>

        {/* ── Left: Configuration Controls ── */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>

          {/* Benchmark Selection */}
          <div className="card">
            <h3><IconTerminal size={16} color="var(--neon-cyan)" /> Benchmark Workload</h3>
            <div className="form-group" style={{ marginTop: '0.85rem' }}>
              <label className="form-label">
                <span>Program Selection</span>
                <span style={{ color: 'var(--neon-cyan)' }}>ASM_MIPS</span>
              </label>
              <select className="select" value={config.benchmark} onChange={e => set('benchmark', e.target.value)}>
                {benchmarks.map(b => <option key={b} value={b}>{b}.asm</option>)}
              </select>
            </div>
          </div>

          {/* Pipeline Configuration */}
          <div className="card">
            <h3><IconPipeline size={16} color="var(--neon-cyan)" /> Pipeline Pipeline Control</h3>
            <div style={{ marginTop: '0.5rem' }}>
              <Toggle checked={config.pipeline.enabled}    onChange={v => set('pipeline.enabled', v)}    label="5-Stage Pipelining" />
              <Toggle checked={config.pipeline.forwarding} onChange={v => set('pipeline.forwarding', v)} label="Data Forwarding (EX/MEM -> EX)" />
              <Toggle checked={config.pipeline.stalling}   onChange={v => set('pipeline.stalling', v)}   label="Hazard Stalling (Load-Use)" />
            </div>
          </div>

          {/* Cache Hierarchy */}
          <div className="card">
            <h3><IconMemory size={16} color="var(--neon-cyan)" /> Cache Hierarchy (L1 Data)</h3>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem', marginTop: '0.85rem' }}>
              <div className="form-group">
                <label className="form-label">
                  <span>Cache Size (KB)</span>
                  <span style={{ color: 'var(--text-sec)' }}>0 = Disabled</span>
                </label>
                <input className="input" type="number" min="0" max="128"
                  value={config.cache.sizeKB}
                  onChange={e => set('cache.sizeKB', parseInt(e.target.value) || 0)} />
              </div>
              <div className="grid-2">
                <div className="form-group">
                  <label className="form-label">Block Size (B)</label>
                  <select className="select" value={config.cache.blockSize} onChange={e => set('cache.blockSize', parseInt(e.target.value))}>
                    {[16, 32, 64, 128].map(v => <option key={v}>{v}</option>)}
                  </select>
                </div>
                <div className="form-group">
                  <label className="form-label">Associativity</label>
                  <select className="select" value={config.cache.associativity} onChange={e => set('cache.associativity', parseInt(e.target.value))}>
                    {[1,2,4,8].map(v => <option key={v} value={v}>{v === 1 ? 'Direct Mapped' : `${v}-Way`}</option>)}
                  </select>
                </div>
              </div>
              <div className="form-group">
                <label className="form-label">Replacement Policy</label>
                <select className="select" value={config.cache.replacement} onChange={e => set('cache.replacement', e.target.value)}>
                  {REPLACEMENTS.map(r => <option key={r}>{r}</option>)}
                </select>
              </div>
            </div>
          </div>

          {/* Branch Predictor */}
          <div className="card">
            <h3><IconPredictor size={16} color="var(--neon-cyan)" /> Branch Prediction</h3>
            <div className="form-group" style={{ marginTop: '0.85rem' }}>
              <label className="form-label">Prediction Algorithm</label>
              <select className="select" value={config.branchPredictor.type} onChange={e => set('branchPredictor.type', e.target.value)}>
                {PREDICTORS.map(p => <option key={p}>{p}</option>)}
              </select>
            </div>
          </div>

          {/* Run Button */}
          <button className="btn btn-primary" style={{ width: '100%', justifyContent: 'center', padding: '0.85rem' }} onClick={run} disabled={loading}>
            {loading ? <><span className="spinner" /> EXECUTING SIMULATION...</> : <><IconPlay size={14} color="#04060a" /> RUN SIMULATION</>}
          </button>

          {error && <div className="alert alert-error">[SYS_ERR] {error}</div>}
        </div>

        {/* ── Right: Execution Telemetry ── */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
          <div className="card card-cyan" style={{ flex: 1 }}>
            <h3><IconMetrics size={16} color="var(--neon-cyan)" /> Simulation Telemetry & Metrics</h3>

            {!result && !loading && (
              <div style={{ textAlign: 'center', padding: '5rem 2rem', color: 'var(--text-sec)' }}>
                <div style={{ display: 'inline-flex', padding: '1rem', border: '1px solid var(--border-tech)', marginBottom: '1rem' }}>
                  <IconConfig size={32} color="var(--text-muted)" />
                </div>
                <p style={{ fontFamily: 'var(--font-mono)', fontSize: '0.85rem' }}>SYSTEM IDLE // Execute a simulation configuration to view execution trace telemetry</p>
              </div>
            )}

            {loading && (
              <div style={{ textAlign: 'center', padding: '5rem 2rem', color: 'var(--neon-cyan)' }}>
                <div className="spinner" style={{ width: 36, height: 36, borderWidth: 3 }} />
                <p style={{ marginTop: '1rem', fontFamily: 'var(--font-mono)', fontSize: '0.85rem' }}>COMPUTING CYCLE-ACCURATE TRACE...</p>
              </div>
            )}

            {result && (
              <div className="fade-in" style={{ marginTop: '1rem' }}>
                <div className="metrics-grid" style={{ marginBottom: '1.5rem' }}>
                  <MetricCard label="Total Cycles" value={result.totalCycles} accentClass="accent-cyan" />
                  <MetricCard label="Instructions" value={result.instructionsExecuted} />
                  <MetricCard label="CPI (Cycles/Instr)" value={cpi} accentClass="accent-cyan" />
                  <MetricCard label="IPC (Instr/Cycle)" value={ipc} accentClass="accent-green" />
                  <MetricCard label="Stall Cycles" value={stalls} accentClass="accent-amber" />
                  <MetricCard label="Cache Hit Rate" value={hitRate} accentClass="accent-green" />
                  <MetricCard label="Branch Mispred" value={mispred} accentClass="accent-pink" />
                </div>

                <div className="divider" />

                <h3 style={{ marginBottom: '0.75rem' }}>Config Vector JSON</h3>
                <pre style={{
                  background: 'var(--bg-input)', padding: '1rem',
                  border: '1px solid var(--border-tech)', fontSize: '0.75rem',
                  fontFamily: 'var(--font-mono)', color: 'var(--neon-cyan)',
                  overflowX: 'auto', lineHeight: 1.6
                }}>{JSON.stringify(config, null, 2)}</pre>

                <button className="btn btn-secondary" style={{ marginTop: '1.25rem', width: '100%', justifyContent: 'center' }}
                  onClick={() => onGoTo('pipeline')}>
                  INSPECT PIPELINE TRACE <IconArrowRight size={14} color="currentColor" />
                </button>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
