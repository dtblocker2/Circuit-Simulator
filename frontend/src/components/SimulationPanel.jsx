import { useState } from 'react';

export default function SimulationPanel({ onSimulate, onClear, loading }) {
  const [t1, setT1] = useState('0');
  const [t2, setT2] = useState('10m');
  const [dt, setDt] = useState('10u');
  const [node, setNode] = useState('1');

  function handleSimulate(e) {
    e.preventDefault();
    onSimulate({ t1, t2, dt, node: Number(node) });
  }

  return (
    <form onSubmit={handleSimulate}>
      <div className="section-title">Simulate &amp; Plot Waveform</div>
      <div className="form-card">
        <div className="sim-grid">
          <div className="form-row">
            <label>Start Time (t1)</label>
            <input value={t1} onChange={(e) => setT1(e.target.value)} />
          </div>
          <div className="form-row">
            <label>End Time (t2)</label>
            <input value={t2} onChange={(e) => setT2(e.target.value)} />
          </div>
          <div className="form-row">
            <label>Time Step (dt)</label>
            <input value={dt} onChange={(e) => setDt(e.target.value)} />
          </div>
          <div className="form-row">
            <label>Target Node ID</label>
            <input value={node} onChange={(e) => setNode(e.target.value)} />
          </div>
        </div>
        <button type="submit" className="btn-primary" disabled={loading}>
          {loading ? 'Running…' : 'Run Simulation'}
        </button>
        <button type="button" className="btn-danger" onClick={onClear}>
          Clear Circuit
        </button>
      </div>
    </form>
  );
}
