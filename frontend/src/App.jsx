import { useState, useEffect, useCallback } from 'react';
import ComponentBuilder from './components/ComponentBuilder';
import SimulationPanel from './components/SimulationPanel';
import ConsolePanel from './components/ConsolePanel';
import Oscilloscope from './components/Oscilloscope';
import {
  checkHealth,
  sendCommand,
  runSimulation,
  clearCircuit,
  subscribeLogs,
} from './api';
import { formatLogLine, trimLogHistory } from './utils/formatLog';

function formatCursor(cursor) {
  if (!cursor) return 'Hover over the graph to inspect voltage and time';
  const ms = cursor.time * 1e3;
  const timeStr = Math.abs(cursor.time) >= 1e-3
    ? `${ms.toFixed(4)} ms`
    : `${(cursor.time * 1e6).toFixed(4)} µs`;
  return `Time: ${timeStr}  |  Voltage: ${cursor.voltage.toFixed(4)} V`;
}

export default function App() {
  const [tab, setTab] = useState('builder');
  const [online, setOnline] = useState(false);
  const [logs, setLogs] = useState([]);
  const [waveform, setWaveform] = useState(null);
  const [cursor, setCursor] = useState(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    checkHealth().then((h) => setOnline(h.ok)).catch(() => setOnline(false));
    const interval = setInterval(() => {
      checkHealth().then((h) => setOnline(h.ok)).catch(() => setOnline(false));
    }, 5000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    const unsubscribe = subscribeLogs((line) => {
      setLogs((prev) => trimLogHistory([...prev, formatLogLine(line)]));
    });
    return unsubscribe;
  }, []);

  const handleCommand = useCallback(async (command) => {
    try {
      const result = await sendCommand(command);
      if (result.waveform) setWaveform(result.waveform);
    } catch (err) {
      setLogs((prev) => [...prev, `[ERROR] ${err.message}`]);
    }
  }, []);

  const handleSimulate = useCallback(async (params) => {
    setLoading(true);
    try {
      const result = await runSimulation(params);
      if (result.waveform) setWaveform(result.waveform);
    } catch (err) {
      setLogs((prev) => [...prev, `[ERROR] ${err.message}`]);
    } finally {
      setLoading(false);
    }
  }, []);

  const handleClear = useCallback(async () => {
    try {
      await clearCircuit();
      setWaveform(null);
      setCursor(null);
    } catch (err) {
      setLogs((prev) => [...prev, `[ERROR] ${err.message}`]);
    }
  }, []);

  const handleMacro = useCallback(async (macro) => {
    setLoading(true);
    try {
      for (const cmd of macro.commands) {
        await sendCommand(cmd);
      }
      const result = await runSimulation(macro.sim);
      if (result.waveform) setWaveform(result.waveform);
    } catch (err) {
      setLogs((prev) => [...prev, `[ERROR] ${err.message}`]);
    } finally {
      setLoading(false);
    }
  }, []);

  return (
    <div className="app">
      <header className="app-header">
        <h1><span>MNA</span> Circuit Studio</h1>
        <div className="status-badge">
          <span className={`status-dot${online ? ' online' : ''}`} />
          {online ? 'C++ Engine Online' : 'Engine Offline — compile sim_engine first'}
        </div>
      </header>

      <div className="app-body">
        <aside className="sidebar">
          <div className="tabs">
            <button
              type="button"
              className={`tab${tab === 'builder' ? ' active' : ''}`}
              onClick={() => setTab('builder')}
            >
              Circuit Builder
            </button>
            <button
              type="button"
              className={`tab${tab === 'console' ? ' active' : ''}`}
              onClick={() => setTab('console')}
            >
              CLI &amp; Logs
            </button>
          </div>

          <div className="tab-content">
            {tab === 'builder' ? (
              <>
                <ComponentBuilder onAdd={handleCommand} />
                <SimulationPanel
                  onSimulate={handleSimulate}
                  onClear={handleClear}
                  loading={loading}
                />
              </>
            ) : (
              <ConsolePanel
                logs={logs}
                onCommand={handleCommand}
                onMacro={handleMacro}
              />
            )}
          </div>
        </aside>

        <main className="main-panel">
          <div className="cursor-info">{formatCursor(cursor)}</div>
          <div className="chart-container">
            <Oscilloscope
              waveform={waveform}
              cursor={cursor}
              onCursorChange={setCursor}
            />
          </div>
        </main>
      </div>
    </div>
  );
}
