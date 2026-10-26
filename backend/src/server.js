const express = require('express');
const cors    = require('cors');
const path    = require('path');
const fs      = require('fs');
const { spawn } = require('child_process');

const app  = express();
const PORT = process.env.PORT || 3001;

// ─── Paths ────────────────────────────────────────────────────────────────────
const ROOT_DIR       = path.resolve(__dirname, '..', '..');
const SIM_EXE        = path.join(ROOT_DIR, 'sim-core', 'build', 'sim.exe');
const BENCHMARKS_DIR = path.join(ROOT_DIR, 'sim-core', 'benchmarks');

// ─── Middleware ───────────────────────────────────────────────────────────────
app.use(cors());
app.use(express.json({ limit: '2mb' }));

// ─── Helper: run sim.exe with JSON input ──────────────────────────────────────
function runSim(jsonInput, extraArgs = []) {
  return new Promise((resolve, reject) => {
    const args = ['--benchmarks-dir', BENCHMARKS_DIR, ...extraArgs];
    const proc = spawn(SIM_EXE, args);

    let stdout = '';
    let stderr = '';
    proc.stdout.on('data', d => { stdout += d; });
    proc.stderr.on('data', d => { stderr += d; });
    proc.on('error', reject);
    proc.on('close', code => {
      if (code !== 0) return reject(new Error(`sim.exe exited ${code}: ${stderr}`));
      try {
        resolve(JSON.parse(stdout));
      } catch (e) {
        reject(new Error(`JSON parse error: ${e.message}\nOutput: ${stdout.slice(0, 500)}`));
      }
    });

    proc.stdin.write(JSON.stringify(jsonInput));
    proc.stdin.end();
  });
}

// ─── Routes ───────────────────────────────────────────────────────────────────

/**
 * GET /health
 * Health check
 */
app.get('/health', (req, res) => {
  const simExists = fs.existsSync(SIM_EXE);
  res.json({
    status: 'ok',
    sim_exe: SIM_EXE,
    sim_ready: simExists,
    benchmarks_dir: BENCHMARKS_DIR,
  });
});

/**
 * GET /benchmarks
 * List available benchmark files
 */
app.get('/benchmarks', (req, res) => {
  try {
    const files = fs.readdirSync(BENCHMARKS_DIR)
      .filter(f => f.endsWith('.asm'))
      .map(f => f.replace('.asm', ''));
    res.json({ benchmarks: files });
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/**
 * POST /simulate
 * Run a single simulation.
 * Body: SimConfig JSON (benchmark, pipeline, cache, branchPredictor)
 */
app.post('/simulate', async (req, res) => {
  try {
    const config = req.body;
    if (!config.benchmark) return res.status(400).json({ error: 'benchmark is required' });
    const result = await runSim(config);
    res.json(result);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/**
 * POST /sweep
 * Run a full parameter sweep.
 * Body: { parallel?: boolean, threads?: number, benchmarks?: string[] }
 * Returns: JSON array of SimResult objects.
 */
app.post('/sweep', async (req, res) => {
  try {
    const body   = req.body || {};
    const input  = { mode: 'sweep', ...body };
    const result = await runSim(input);
    res.json(result);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

/**
 * POST /sweep/parallel
 * Same as /sweep but forces parallel=true.
 */
app.post('/sweep/parallel', async (req, res) => {
  try {
    const body   = req.body || {};
    const input  = { mode: 'sweep', parallel: true, ...body };
    const result = await runSim(input);
    res.json(result);
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

// ─── Start ────────────────────────────────────────────────────────────────────
app.listen(PORT, () => {
  console.log(`✅ Processor Design Studio backend listening on port ${PORT}`);
  console.log(`   sim.exe: ${SIM_EXE}`);
  console.log(`   benchmarks: ${BENCHMARKS_DIR}`);
});
