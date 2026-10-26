import { useState } from 'react';
import { API } from '../App';
import { 
  IconDashboard, 
  IconParallel, 
  IconPlay, 
  IconMetrics, 
  IconTerminal, 
  IconConfig, 
  IconPipeline, 
  IconMemory, 
  IconPredictor 
} from '../components/Icons';

const BENCHMARKS   = ['loop_sum', 'factorial', 'bubble_sort', 'array_sum', 'hazard_heavy'];
const PREDICTORS   = ['STATIC_NOT_TAKEN', 'STATIC_TAKEN', 'ONE_BIT', 'TWO_BIT_SATURATING'];
const REPLACEMENTS = ['LRU', 'FIFO', 'Random'];

const CYBER_PALETTE = [
  '#00f0ff', '#00ff66', '#ffcc00', '#ff0055', '#7000ff',
  '#33f3ff', '#33ff85', '#ffdd33', '#ff3377', '#9933ff',
];

function barColor(idx) { return CYBER_PALETTE[idx % CYBER_PALETTE.length]; }

// ── Inline Brutalist Bar Chart ──────────────────────────────────────────────
function BarChart({ rows, title, unit = '' }) {
  const max = Math.max(...rows.map(r => r.value), 0.001);
  return (
    <div>
      {title && <h3 style={{ marginBottom: '1.25rem', fontFamily: 'var(--font-mono)' }}>{title}</h3>}
      <div className="bar-chart">
        {rows.map((r, i) => (
          <div className="bar-row" key={r.label}>
            <span className="bar-label" title={r.label}>
              {r.label.length > 22 ? r.label.slice(0, 20) + '…' : r.label}
            </span>
            <div className="bar-track">
              <div
                className="bar-fill"
                style={{ width: `${(r.value / max) * 100}%`, background: barColor(i) }}
              />
            </div>
            <span className="bar-val">{r.value.toFixed(3)}{unit}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

// ── Telemetry Summary Strip ──────────────────────────────────────────────────
function SummaryStrip({ entries }) {
  if (!entries.length) return null;
  const best   = entries.reduce((a, b) => a.cpi < b.cpi ? a : b);
  const worst  = entries.reduce((a, b) => a.cpi > b.cpi ? a : b);
  const avgCpi = entries.reduce((s, e) => s + e.cpi, 0) / entries.length;

  return (
    <div className="grid-4" style={{ marginBottom: '1.5rem' }}>
      {[
        { label: 'Configurations Run', value: entries.length,           sub: 'TOTAL SWEEP ITERATIONS',        accentClass: 'accent-cyan' },
        { label: 'Optimum CPI',        value: best.cpi.toFixed(3),      sub: `${best.benchmark} · ${best.pipeline.enabled ? 'PIPELINED' : 'FUNC'}`, accentClass: 'accent-green' },
        { label: 'Maximum CPI',        value: worst.cpi.toFixed(3),     sub: worst.benchmark,                 accentClass: 'accent-pink' },
        { label: 'Mean CPI',           value: avgCpi.toFixed(3),        sub: 'CROSS-CONFIG AVERAGE',          accentClass: 'accent-amber' },
      ].map(m => (
        <div className={`metric ${m.accentClass}`} key={m.label}>
          <span className="metric-label">{m.label}</span>
          <span className="metric-value">{m.value}</span>
          <span className="metric-sub">{m.sub}</span>
        </div>
      ))}
    </div>
  );
}

// ── Results Data Matrix ──────────────────────────────────────────────────────
function ResultsTable({ entries, sortKey, onSort }) {
  const cols = [
    { key: 'benchmark',              label: 'Workload' },
    { key: 'pipeline',               label: 'Pipeline' },
    { key: 'branchPredictor',        label: 'Predictor' },
    { key: 'cache',                  label: 'L1 Cache' },
    { key: 'cpi',                    label: 'CPI' },
    { key: 'ipc',                    label: 'IPC' },
    { key: 'stallCycles',            label: 'Stalls' },
    { key: 'cacheHitRate',           label: 'Hit %' },
    { key: 'branchMispredictionRate', label: 'Mispred %' },
    { key: 'wallTimeMs',             label: 'Time (ms)' },
  ];

  return (
    <div className="table-wrap">
      <table>
        <thead>
          <tr>
            {cols.map(c => (
              <th key={c.key} onClick={() => onSort(c.key)} style={{ cursor: 'pointer', userSelect: 'none' }}>
                {c.label} {sortKey === c.key ? '▲' : ''}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {entries.map((e, i) => (
            <tr key={i}>
              <td><span className="badge badge-cyan">{e.benchmark}</span></td>
              <td>
                {e.pipeline.enabled
                  ? <span className="badge badge-green">{e.pipeline.forwarding ? 'FORWARDING' : 'STALL-ONLY'}</span>
                  : <span className="badge badge-amber">FUNCTIONAL</span>}
              </td>
              <td style={{ fontSize: '0.72rem', color: 'var(--text-sec)' }}>{e.branchPredictor?.type || '—'}</td>
              <td style={{ fontSize: '0.72rem', color: 'var(--text-sec)' }}>
                {e.cache?.sizeKB > 0 ? `${e.cache.sizeKB}KB · ${e.cache.associativity}-Way · ${e.cache.replacement}` : 'NONE'}
              </td>
              <td style={{ color: 'var(--neon-cyan)', fontWeight: 700 }}>{e.cpi.toFixed(3)}</td>
              <td style={{ color: 'var(--neon-green)' }}>{e.ipc.toFixed(3)}</td>
              <td style={{ color: e.stallCycles > 0 ? 'var(--neon-amber)' : 'var(--text-muted)' }}>{e.stallCycles}</td>
              <td style={{ color: 'var(--neon-green)' }}>
                {e.cacheHitRate > 0 ? (e.cacheHitRate * 100).toFixed(1) + '%' : '—'}
              </td>
              <td style={{ color: e.branchMispredictionRate > 0.1 ? 'var(--neon-magenta)' : 'var(--neon-green)' }}>
                {(e.branchMispredictionRate * 100).toFixed(1)}%
              </td>
              <td style={{ color: 'var(--text-muted)' }}>
                {e.wallTimeMs != null ? e.wallTimeMs.toFixed(2) : '—'}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

// ── Main Dashboard Component ────────────────────────────────────────────────
export default function Dashboard() {
  const [entries,    setEntries]    = useState([]);
  const [loading,    setLoading]    = useState(false);
  const [error,      setError]      = useState('');
  const [parallel,   setParallel]   = useState(false);
  const [sortKey,    setSortKey]    = useState('cpi');
  const [filterBench, setFilterBench] = useState('all');
  const [filterPred,  setFilterPred]  = useState('all');
  const [chartMetric, setChartMetric] = useState('cpi');

  async function runSweep() {
    setLoading(true); setError(''); setEntries([]);
    try {
      const endpoint = parallel ? `${API}/sweep/parallel` : `${API}/sweep`;
      const res = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ parallel }),
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Sweep failed');
      if (!Array.isArray(data)) throw new Error('Expected array response');
      setEntries(data);
    } catch (e) {
      setError(e.message);
    } finally {
      setLoading(false);
    }
  }

  // Filter & sort
  const filtered = entries
    .filter(e => filterBench === 'all' || e.benchmark === filterBench)
    .filter(e => filterPred  === 'all' || e.branchPredictor?.type === filterPred)
    .sort((a, b) => {
      const va = typeof a[sortKey] === 'number' ? a[sortKey] : 0;
      const vb = typeof b[sortKey] === 'number' ? b[sortKey] : 0;
      return va - vb;
    });

  // Chart rows: top 20 by chosen metric
  const chartRows = filtered.slice(0, 20).map((e) => ({
    label: `${e.benchmark}·${e.pipeline.enabled ? (e.pipeline.forwarding ? 'fwd' : 'stall') : 'func'}`,
    value: typeof e[chartMetric] === 'number' ? e[chartMetric] : 0,
  }));

  // CPI by predictor chart
  const cpiByPred = PREDICTORS.map(pt => {
    const subset = filtered.filter(e => e.branchPredictor?.type === pt && e.pipeline.enabled);
    const avg = subset.length ? subset.reduce((s, e) => s + e.cpi, 0) / subset.length : 0;
    return { label: pt.replace('STATIC_', 'S.').replace('_SATURATING', '_SAT'), value: avg };
  }).filter(r => r.value > 0);

  // Mispred by predictor chart
  const mispredByPred = PREDICTORS.map(pt => {
    const subset = filtered.filter(e => e.branchPredictor?.type === pt && e.pipeline.enabled);
    const avg = subset.length ? subset.reduce((s, e) => s + e.branchMispredictionRate, 0) / subset.length : 0;
    return { label: pt.replace('STATIC_', 'S.').replace('_SATURATING', '_SAT'), value: avg };
  }).filter(r => r.value > 0);

  return (
    <div className="fade-in">
      <div className="section-header">
        <div className="section-icon">
          <IconDashboard size={22} color="var(--neon-cyan)" />
        </div>
        <div>
          <h1><span className="gradient-text">Architectural Sweep & Comparison Dashboard</span></h1>
          <p style={{ color: 'var(--text-sec)', fontSize: '0.85rem', fontFamily: 'var(--font-mono)' }}>
            PARALLEL_SWEEP // Multi-dimensional OpenMP parameter exploration
          </p>
        </div>
      </div>

      {/* Sweep Launcher Control */}
      <div className="card" style={{ marginBottom: '1rem' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flexWrap: 'wrap' }}>
          <div>
            <div className="form-label" style={{ marginBottom: 6 }}>OpenMP Multi-Threading</div>
            <div style={{ display: 'flex', gap: 6 }}>
              <button className={`btn ${!parallel ? 'btn-primary' : 'btn-secondary'}`}
                onClick={() => setParallel(false)}>SEQUENTIAL</button>
              <button className={`btn ${parallel ? 'btn-primary' : 'btn-secondary'}`}
                onClick={() => setParallel(true)}>
                <IconParallel size={14} color="currentColor" /> OPENMP PARALLEL
              </button>
            </div>
          </div>
          <button
            className="btn btn-primary"
            style={{ marginLeft: 'auto', padding: '0.85rem 2rem' }}
            onClick={runSweep}
            disabled={loading}
          >
            {loading
              ? <><span className="spinner" /> EXECUTING SWEEP...</>
              : <><IconPlay size={14} color="#04060a" /> RUN FULL PARAMETER SWEEP</>}
          </button>
        </div>

        {loading && (
          <div style={{ marginTop: '1rem' }}>
            <div className="alert alert-info">
              [SWEEP_EXEC] Executing {parallel ? 'OpenMP multithreaded parallel' : 'sequential'} simulation sweep grid across all benchmarks…
            </div>
          </div>
        )}
        {error && <div className="alert alert-error" style={{ marginTop: '1rem' }}>[SYS_ERR] {error}</div>}
      </div>

      {entries.length > 0 && (
        <>
          {/* Summary metrics */}
          <SummaryStrip entries={filtered} />

          {/* Filters */}
          <div className="card" style={{ marginBottom: '1rem' }}>
            <div style={{ display: 'flex', gap: '1rem', flexWrap: 'wrap', alignItems: 'flex-end' }}>
              <div className="form-group" style={{ minWidth: 170 }}>
                <label className="form-label">Filter: Workload</label>
                <select className="select" value={filterBench} onChange={e => setFilterBench(e.target.value)}>
                  <option value="all">ALL WORKLOADS</option>
                  {BENCHMARKS.map(b => <option key={b} value={b}>{b}.asm</option>)}
                </select>
              </div>
              <div className="form-group" style={{ minWidth: 210 }}>
                <label className="form-label">Filter: Predictor</label>
                <select className="select" value={filterPred} onChange={e => setFilterPred(e.target.value)}>
                  <option value="all">ALL PREDICTORS</option>
                  {PREDICTORS.map(p => <option key={p}>{p}</option>)}
                </select>
              </div>
              <div className="form-group" style={{ minWidth: 180 }}>
                <label className="form-label">Chart Metric Axis</label>
                <select className="select" value={chartMetric} onChange={e => setChartMetric(e.target.value)}>
                  <option value="cpi">CPI (Cycles / Instr)</option>
                  <option value="ipc">IPC (Instr / Cycle)</option>
                  <option value="stallCycles">Stall Cycles</option>
                  <option value="branchMispredictionRate">Misprediction Rate</option>
                  <option value="cacheHitRate">Cache Hit Rate</option>
                  <option value="wallTimeMs">Wall Time (ms)</option>
                </select>
              </div>
              <span className="badge badge-cyan" style={{ marginLeft: 'auto', alignSelf: 'center' }}>
                {filtered.length} CONFIGS MATCHED
              </span>
            </div>
          </div>

          {/* Charts grid */}
          <div className="grid-2" style={{ marginBottom: '1.5rem', gap: '1rem' }}>
            <div className="card card-cyan">
              <BarChart rows={chartRows} title={`TELEMETRY: ${chartMetric.toUpperCase()} — TOP CONFIGURATIONS`} />
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
              {cpiByPred.length > 0 && (
                <div className="card">
                  <BarChart rows={cpiByPred} title="MEAN CPI BY BRANCH PREDICTOR" />
                </div>
              )}
              {mispredByPred.length > 0 && (
                <div className="card">
                  <BarChart rows={mispredByPred} title="MISPREDICTION RATE BY PREDICTOR" />
                </div>
              )}
            </div>
          </div>

          {/* Data table */}
          <div className="card">
            <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '1rem' }}>
              <h3><IconMetrics size={16} color="var(--neon-cyan)" /> Complete Sweep Matrix Data</h3>
              <span style={{ fontSize: '0.72rem', fontFamily: 'var(--font-mono)', color: 'var(--text-sec)' }}>
                SELECT COLUMN HEADER TO SORT MATRIX
              </span>
            </div>
            <ResultsTable entries={filtered} sortKey={sortKey} onSort={setSortKey} />
          </div>
        </>
      )}

      {!entries.length && !loading && (
        <div className="card card-cyan" style={{ textAlign: 'center', padding: '4rem 2rem' }}>
          <div style={{ display: 'inline-flex', padding: '1.25rem', border: '1px solid var(--neon-cyan)', marginBottom: '1rem' }}>
            <IconDashboard size={40} color="var(--neon-cyan)" />
          </div>
          <h2 style={{ marginBottom: '0.5rem', color: 'var(--text-main)' }}>SYSTEM READY FOR PARALLEL SWEEP</h2>
          <p style={{ color: 'var(--text-sec)', maxWidth: 540, margin: '0 auto 2rem', fontFamily: 'var(--font-mono)', fontSize: '0.82rem' }}>
            Execute multi-parameter sweep combinations across benchmarks, 5-stage pipeline modes,
            cache configurations, and branch predictor algorithms.
          </p>
          <div className="grid-3" style={{ maxWidth: 680, margin: '0 auto', textAlign: 'left' }}>
            {[
              { Icon: IconTerminal,  label: '5 Benchmark Workloads', desc: 'loop_sum, factorial, bubble_sort…' },
              { Icon: IconPipeline,  label: '3 Pipeline Modes',      desc: 'functional, pipelined+fwd, stall-only' },
              { Icon: IconMemory,    label: '6 Cache Configurations', desc: '1KB DM -> 4KB 4-way FIFO/LRU/Rand' },
              { Icon: IconPredictor, label: '4 Predictor Types',     desc: 'Static, 1-bit, 2-bit saturating' },
              { Icon: IconParallel,  label: 'OpenMP Multithreading', desc: 'Multi-threaded sweep execution' },
              { Icon: IconMetrics,   label: 'Telemetry Charts',      desc: 'CPI, IPC, misprediction, cache hit %' },
            ].map(f => (
              <div key={f.label} style={{
                padding: '0.85rem', background: 'var(--bg-input)',
                border: '1px solid var(--border-tech)',
              }}>
                <div style={{ marginBottom: '0.5rem', color: 'var(--neon-cyan)' }}><f.Icon size={20} color="currentColor" /></div>
                <div style={{ fontWeight: 700, fontSize: '0.8rem', fontFamily: 'var(--font-mono)' }}>{f.label}</div>
                <div style={{ fontSize: '0.72rem', color: 'var(--text-sec)', fontFamily: 'var(--font-mono)', marginTop: 2 }}>{f.desc}</div>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
