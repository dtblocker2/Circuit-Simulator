import { useState, useRef, useEffect } from 'react';
import { MAX_LOG_LINES } from '../utils/formatLog';

const MACROS = [
  {
    label: 'CMOS Inverter Demo',
    commands: [
      'CLEAR',
      'ADD_GATE G1 NOT 2 1 5 0',
      'ADD_STEP_V V1 1 0 0 5 2u',
    ],
    sim: { t1: '0', t2: '10u', dt: '10n', node: 2 },
  },
  {
    label: 'Diode Rectifier Demo',
    commands: [
      'CLEAR',
      'ADD_STEP_V V1 1 0 -5 5 2m',
      'ADD_DIODE D1 1 2',
      'ADD_R R1 2 0 1k',
    ],
    sim: { t1: '0', t2: '5m', dt: '5u', node: 2 },
  },
  {
    label: 'RC Low-Pass Filter',
    commands: [
      'CLEAR',
      'ADD_STEP_V V1 1 0 0 5 1m',
      'ADD_R R1 1 2 1k',
      'ADD_C C1 2 0 100u',
    ],
    sim: { t1: '0', t2: '20m', dt: '10u', node: 2 },
  },
];

export default function ConsolePanel({ logs, onCommand, onMacro }) {
  const [cliInput, setCliInput] = useState('');
  const logPanelRef = useRef(null);
  const logEndRef = useRef(null);

  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }, [logs]);

  function handleSubmit(e) {
    e.preventDefault();
    if (cliInput.trim()) {
      onCommand(cliInput.trim());
      setCliInput('');
    }
  }

  return (
    <div className="console">
      <div className="section-title">Demo Circuits</div>
      <div className="macro-list">
        {MACROS.map((macro) => (
          <button
            key={macro.label}
            type="button"
            className="btn-secondary"
            onClick={() => onMacro(macro)}
          >
            {macro.label}
          </button>
        ))}
      </div>

      <div className="section-title">Manual CLI</div>
      <form className="cli-row" onSubmit={handleSubmit}>
        <input
          value={cliInput}
          onChange={(e) => setCliInput(e.target.value)}
          placeholder="ADD_R R1 1 0 1k"
        />
        <button type="submit">Run</button>
      </form>

      <div className="section-title">
        Engine Logs
        <span className="log-count">{logs.length}/{MAX_LOG_LINES}</span>
      </div>
      <div className="log-panel" ref={logPanelRef}>
        {logs.length === 0 ? (
          <div className="log-empty">No engine output yet</div>
        ) : (
          logs.map((line, i) => (
            <div
              key={`${i}-${line.slice(0, 24)}`}
              className={`log-line${line.includes('[ERROR]') ? ' error' : line.includes('[OK]') ? ' ok' : ''}`}
              title={line}
            >
              {line}
            </div>
          ))
        )}
        <div ref={logEndRef} />
      </div>
    </div>
  );
}

export { MACROS };
