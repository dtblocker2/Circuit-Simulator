const API = '/api';

export async function checkHealth() {
  const res = await fetch(`${API}/health`);
  return res.json();
}

export async function sendCommand(command) {
  const res = await fetch(`${API}/command`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ command }),
  });
  if (!res.ok) {
    const err = await res.json();
    throw new Error(err.error || 'Command failed');
  }
  return res.json();
}

export async function runSimulation({ t1, t2, dt, node }) {
  const res = await fetch(`${API}/simulate`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ t1, t2, dt, node }),
  });
  if (!res.ok) {
    const err = await res.json();
    throw new Error(err.error || 'Simulation failed');
  }
  return res.json();
}

export async function clearCircuit() {
  const res = await fetch(`${API}/clear`, { method: 'POST' });
  if (!res.ok) {
    const err = await res.json();
    throw new Error(err.error || 'Clear failed');
  }
  return res.json();
}

export function subscribeLogs(onLog) {
  const source = new EventSource(`${API}/logs/stream`);
  source.onmessage = (e) => {
    try {
      onLog(JSON.parse(e.data));
    } catch {
      onLog(e.data);
    }
  };
  return () => source.close();
}
