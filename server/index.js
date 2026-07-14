import { spawn } from 'child_process';
import cors from 'cors';
import express from 'express';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, '..');
const ENGINE_EXE = path.join(ROOT, process.platform === 'win32' ? 'sim_engine.exe' : 'sim_engine');

const app = express();
const PORT = process.env.PORT || 3000;
const MAX_LOG_LINES = 200;
const MAX_LOG_CHARS = 120;

function formatLogLine(line) {
  if (line.startsWith('[OSC_DATA]')) {
    try {
      const data = JSON.parse(line.slice('[OSC_DATA]'.length).trim());
      return `[OSC_DATA] node=${data.node} points=${data.times?.length ?? 0} range=[${data.t_start}, ${data.t_end}]`;
    } catch {
      return '[OSC_DATA] (waveform exported)';
    }
  }
  if (line.length > MAX_LOG_CHARS) return `${line.slice(0, MAX_LOG_CHARS)}…`;
  return line;
}

app.use(cors());
app.use(express.json());

class SimEngine {
  constructor() {
    this.proc = null;
    this.logs = [];
    this.listeners = new Set();
    this.commandQueue = [];
    this.currentResolve = null;
    this.responseBuffer = [];
    this.stdoutBuffer = '';
    this.lastWaveform = null;
  }

  isAlive() {
    return this.proc && !this.proc.killed && this.proc.exitCode === null && !this.proc.stdin?.destroyed;
  }

  start() {
    if (!fs.existsSync(ENGINE_EXE)) {
      throw new Error(
        `C++ engine not found at ${ENGINE_EXE}. Compile with: g++ -std=c++17 -O3 main.cpp -o sim_engine`
      );
    }

    if (this.proc) {
      this.proc.removeAllListeners();
      try { this.proc.stdin?.destroy(); } catch { /* ignore */ }
      try { this.proc.kill(); } catch { /* ignore */ }
    }

    this.proc = spawn(ENGINE_EXE, [], {
      cwd: ROOT,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    this.stdoutBuffer = '';

    this.proc.stdout.on('data', (chunk) => {
      this.stdoutBuffer += chunk.toString();
      const parts = this.stdoutBuffer.split(/\r?\n/);
      this.stdoutBuffer = parts.pop() ?? '';
      for (const line of parts) {
        if (!line) continue;
        this.processLine(line);
      }
    });

    this.proc.stderr.on('data', (chunk) => {
      const text = chunk.toString().trim();
      if (text) this.appendLog(`[stderr] ${text}`);
    });

    this.proc.stdin?.on('error', () => {
      this.failCurrentCommand('Engine stdin closed unexpectedly');
    });

    this.proc.on('error', (err) => {
      this.appendLog(`[ERROR] Engine process error: ${err.message}`);
      this.failCurrentCommand(err.message);
    });

    this.proc.on('exit', (code) => {
      this.appendLog(`[STATUS] Engine exited with code ${code ?? 'unknown'}`);
      this.failCurrentCommand('Engine process terminated');
    });
  }

  appendLog(line) {
    const display = formatLogLine(line);
    this.logs.push(display);
    if (this.logs.length > MAX_LOG_LINES) this.logs.shift();
    this.emit('log', display);
  }

  failCurrentCommand(message) {
    if (this.currentResolve) {
      const resolve = this.currentResolve;
      this.currentResolve = null;
      this.responseBuffer = [];
      resolve(['[ERROR] ' + message]);
    }
  }

  processLine(line) {
    this.appendLog(line);

    if (line.startsWith('[OSC_DATA]')) {
      const jsonStr = line.slice('[OSC_DATA]'.length).trim();
      try {
        this.lastWaveform = JSON.parse(jsonStr);
      } catch {
        this.lastWaveform = null;
      }
    }

    if (this.currentResolve) {
      this.responseBuffer.push(line);
      if (this.isResponseComplete(line)) {
        const lines = [...this.responseBuffer];
        this.responseBuffer = [];
        const resolve = this.currentResolve;
        this.currentResolve = null;
        resolve(lines);
      }
    }
  }

  isResponseComplete(line) {
    return (
      line.startsWith('[OK]') ||
      line.startsWith('[ERROR]') ||
      line.startsWith('[STATUS]') ||
      line.startsWith('[OSC_DATA]') ||
      line.startsWith('[OSC]')
    );
  }

  on(event, fn) {
    if (event === 'log') this.listeners.add(fn);
    return () => this.listeners.delete(fn);
  }

  emit(event, data) {
    if (event === 'log') {
      for (const fn of this.listeners) fn(data);
    }
  }

  executeJob(job) {
    this.writeCommand(
      job.cmd,
      (lines) => {
        job.resolve(lines);
        this.drainQueue();
      },
      (err) => {
        job.reject(err);
        this.drainQueue();
      },
      job.retried
    );
  }

  drainQueue() {
    if (this.currentResolve || this.commandQueue.length === 0) return;
    this.executeJob(this.commandQueue.shift());
  }

  writeCommand(cmd, resolve, reject, retried = false) {
    if (!this.isAlive()) {
      if (!retried) {
        try {
          this.start();
          this.writeCommand(cmd, resolve, reject, true);
        } catch (err) {
          reject(err);
        }
        return;
      }
      reject(new Error('Engine not running'));
      return;
    }

    this.currentResolve = resolve;
    this.responseBuffer = [];

    this.proc.stdin.write(cmd, (err) => {
      if (err) {
        this.currentResolve = null;
        if (!retried) {
          try {
            this.start();
            this.writeCommand(cmd, resolve, reject, true);
          } catch (restartErr) {
            reject(restartErr);
          }
        } else {
          reject(err);
        }
      }
    });
  }

  send(command) {
    const cmd = command.endsWith('\n') ? command : `${command}\n`;

    return new Promise((resolve, reject) => {
      const job = { cmd, resolve, reject, retried: false };
      if (this.currentResolve || this.commandQueue.length > 0) {
        this.commandQueue.push(job);
      } else {
        this.executeJob(job);
      }
    });
  }

  async sendBatch(commands) {
    const results = [];
    for (const cmd of commands) {
      const lines = await this.send(cmd);
      results.push({ command: cmd.trim(), lines });
    }
    return results;
  }
}

const engine = new SimEngine();

try {
  engine.start();
} catch (err) {
  console.error(err.message);
}

app.get('/api/health', (_req, res) => {
  const engineExists = fs.existsSync(ENGINE_EXE);
  res.json({
    ok: engineExists && engine.isAlive(),
    enginePath: ENGINE_EXE,
    engineExists,
  });
});

app.get('/api/logs', (_req, res) => {
  res.json({ logs: engine.logs });
});

app.get('/api/logs/stream', (req, res) => {
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');
  res.flushHeaders();

  for (const line of engine.logs.slice(-50)) {
    res.write(`data: ${JSON.stringify(line)}\n\n`);
  }

  const unsubscribe = engine.on('log', (line) => {
    res.write(`data: ${JSON.stringify(line)}\n\n`);
  });

  req.on('close', () => unsubscribe());
});

app.post('/api/command', async (req, res) => {
  try {
    const { command } = req.body;
    if (!command || typeof command !== 'string') {
      return res.status(400).json({ error: 'command string required' });
    }
    const lines = await engine.send(command);
    res.json({ lines, waveform: engine.lastWaveform ?? null });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/simulate', async (req, res) => {
  try {
    const { t1 = '0', t2 = '10m', dt = '10u', node = 1 } = req.body;
    const results = await engine.sendBatch([
      `SIM ${t1} ${t2} ${dt}`,
      `OSC_JSON ${node}`,
    ]);
    res.json({
      results,
      waveform: engine.lastWaveform ?? null,
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/api/clear', async (_req, res) => {
  try {
    const lines = await engine.send('CLEAR');
    engine.lastWaveform = null;
    res.json({ lines });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.listen(PORT, () => {
  console.log(`Circuit Simulator API running on http://localhost:${PORT}`);
  console.log(`Engine: ${ENGINE_EXE}`);
});
