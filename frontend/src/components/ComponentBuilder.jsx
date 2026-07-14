import React from 'react';

const COMPONENT_TYPES = [
  { id: 'R', label: 'Resistor (R)', cmd: 'ADD_R' },
  { id: 'C', label: 'Capacitor (C)', cmd: 'ADD_C' },
  { id: 'L', label: 'Inductor (L)', cmd: 'ADD_L' },
  { id: 'D', label: 'Diode (D)', cmd: 'ADD_DIODE' },
  { id: 'BJT', label: 'BJT Transistor', cmd: 'ADD_BJT' },
  { id: 'MOSFET', label: 'MOSFET Transistor', cmd: 'ADD_MOSFET' },
  { id: 'GATE', label: 'Logic Gate', cmd: 'ADD_GATE' },
  { id: 'V', label: 'Step Voltage Source', cmd: 'ADD_STEP_V' },
];

const FIELD_CONFIG = {
  R: { n1: 'Node 1 (+)', n2: 'Node 2 (-)', n3: null, val: 'Value (e.g. 10k, 100u)', valDefault: '1k' },
  C: { n1: 'Node 1 (+)', n2: 'Node 2 (-)', n3: null, val: 'Value (SI: k, u, n, Meg)', valDefault: '100u' },
  L: { n1: 'Node 1 (+)', n2: 'Node 2 (-)', n3: null, val: 'Value (e.g. 1m)', valDefault: '1m' },
  D: { n1: 'Anode (+)', n2: 'Cathode (-)', n3: null, val: null, valDefault: '' },
  BJT: { n1: 'Collector', n2: 'Base', n3: 'Emitter', val: 'Type (NPN / PNP)', valDefault: 'NPN' },
  MOSFET: { n1: 'Drain', n2: 'Gate', n3: 'Source', val: 'Type (NMOS / PMOS)', valDefault: 'NMOS' },
  GATE: { n1: 'Output Node', n2: 'Input 1', n3: 'Input 2 (0 if NOT)', val: 'Type Vhigh Vlow', valDefault: 'NOT 5 0' },
  V: { n1: 'Node (+)', n2: 'Node (-)', n3: null, val: 'Vstart Vend Tstep', valDefault: '0 5 1m' },
};

export default function ComponentBuilder({ onAdd }) {
  const [type, setType] = React.useState('R');
  const [name, setName] = React.useState('R1');
  const [n1, setN1] = React.useState('1');
  const [n2, setN2] = React.useState('0');
  const [n3, setN3] = React.useState('0');
  const [val, setVal] = React.useState('1k');

  const fields = FIELD_CONFIG[type];

  function handleTypeChange(newType) {
    setType(newType);
    const defaults = FIELD_CONFIG[newType];
    setVal(defaults.valDefault);
    if (newType === 'R') setName('R1');
    else if (newType === 'C') setName('C1');
    else if (newType === 'L') setName('L1');
    else if (newType === 'D') setName('D1');
    else if (newType === 'BJT') setName('Q1');
    else if (newType === 'MOSFET') setName('M1');
    else if (newType === 'GATE') setName('G1');
    else if (newType === 'V') setName('V1');
  }

  function buildCommand() {
    if (!name.trim() || !n1.trim() || !n2.trim()) {
      alert('Please provide a component name and at least two node IDs.');
      return null;
    }

    switch (type) {
      case 'R':
        return `ADD_R ${name} ${n1} ${n2} ${val}`;
      case 'C':
        return `ADD_C ${name} ${n1} ${n2} ${val}`;
      case 'L':
        return `ADD_L ${name} ${n1} ${n2} ${val}`;
      case 'D':
        return `ADD_DIODE ${name} ${n1} ${n2}`;
      case 'BJT':
        return `ADD_BJT ${name} ${val} ${n1} ${n2} ${n3}`;
      case 'MOSFET':
        return `ADD_MOSFET ${name} ${val} ${n1} ${n2} ${n3}`;
      case 'GATE': {
        const parts = val.split(/\s+/);
        if (parts.length < 3) {
          alert('Logic Gate value must be: TYPE V_HIGH V_LOW (e.g. NAND 5 0)');
          return null;
        }
        const [gType, vh, vl] = parts;
        return gType === 'NOT'
          ? `ADD_GATE ${name} ${gType} ${n1} ${n2} ${vh} ${vl}`
          : `ADD_GATE ${name} ${gType} ${n1} ${n2} ${n3} ${vh} ${vl}`;
      }
      case 'V': {
        const parts = val.split(/\s+/);
        if (parts.length < 3) {
          alert('Step Voltage value must be: V_START V_END T_STEP (e.g. 0 5 1m)');
          return null;
        }
        return `ADD_STEP_V ${name} ${n1} ${n2} ${parts[0]} ${parts[1]} ${parts[2]}`;
      }
      default:
        return null;
    }
  }

  function handleSubmit(e) {
    e.preventDefault();
    const cmd = buildCommand();
    if (cmd) onAdd(cmd);
  }

  return (
    <form onSubmit={handleSubmit}>
      <div className="section-title">Add Circuit Component</div>
      <div className="form-card">
        <div className="form-grid">
          <div className="form-row">
            <label>Component Type</label>
            <select value={type} onChange={(e) => handleTypeChange(e.target.value)}>
              {COMPONENT_TYPES.map((t) => (
                <option key={t.id} value={t.id}>{t.label}</option>
              ))}
            </select>
          </div>
          <div className="form-row">
            <label>Unique Name</label>
            <input value={name} onChange={(e) => setName(e.target.value)} />
          </div>
          <div className="form-row">
            <label>{fields.n1}</label>
            <input value={n1} onChange={(e) => setN1(e.target.value)} />
          </div>
          <div className="form-row">
            <label>{fields.n2}</label>
            <input value={n2} onChange={(e) => setN2(e.target.value)} />
          </div>
          {fields.n3 && (
            <div className="form-row">
              <label>{fields.n3}</label>
              <input value={n3} onChange={(e) => setN3(e.target.value)} />
            </div>
          )}
          {fields.val && (
            <div className="form-row">
              <label>{fields.val}</label>
              <input value={val} onChange={(e) => setVal(e.target.value)} />
            </div>
          )}
        </div>
        <button type="submit" className="btn-primary">Add Component</button>
      </div>
    </form>
  );
}
