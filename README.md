# MNA Circuit Simulator

A full-stack circuit simulator with a **React** frontend, **Node.js** API bridge, and **C++** Modified Nodal Analysis (MNA) simulation engine.

## Architecture

```
┌─────────────────┐     HTTP/SSE      ┌─────────────────┐    stdin/stdout    ┌──────────────────┐
│  React Frontend │ ◄──────────────► │  Express Server │ ◄────────────────► │  C++ sim_engine  │
│  (Vite + Recharts)│                  │  (port 3001)    │                    │  (MNA + Newton)  │
└─────────────────┘                   └─────────────────┘                    └──────────────────┘
```

## Features

- **Passive components**: Resistors, capacitors, inductors
- **Semiconductors**: Diodes, BJT (NPN/PNP), MOSFET (NMOS/PMOS)
- **Logic gates**: NOT, AND, OR, NAND, NOR, XOR
- **Sources**: Step voltage sources with SI unit parsing (k, u, n, m, Meg)
- **Transient simulation** with Newton-Raphson nonlinear convergence
- **Interactive oscilloscope** with hover cursor readout
- **Demo circuits**: CMOS inverter, diode rectifier, RC low-pass filter

## Prerequisites

- **C++ compiler** with C++17 support (MinGW g++ on Windows, or MSVC/clang)
- **Node.js** 18+

## Quick Start

### 1. Compile the C++ engine

```bash
# Windows (MinGW)
g++ -std=c++17 -O3 main.cpp -o sim_engine.exe

# Linux / macOS
g++ -std=c++17 -O3 main.cpp -o sim_engine
```

### 2. Install dependencies

```bash
npm run install:all
```

### 3. Start the server and frontend

In one terminal:
```bash
npm run server
```

In another terminal:
```bash
npm run frontend
```

Open **http://localhost:5173** in your browser.

## Project Structure

```
Circuit Simulator/
├── main.cpp           # C++ CLI engine (stdin command interface)
├── circuit.h          # Circuit class & transient simulator
├── components.h       # R, L, C, diode, BJT, MOSFET, logic gates
├── solver.h           # Gaussian elimination linear solver
├── oscilloscope.h     # Waveform recording & JSON/PPM export
├── si_parser.h        # SI unit suffix parser (1k, 100u, 10m, etc.)
├── app.py             # Legacy Python/Tkinter GUI (optional)
├── server/            # Node.js Express API
│   └── index.js       # Spawns C++ engine, REST + SSE endpoints
└── frontend/          # React + Vite UI
    └── src/
        ├── App.jsx
        └── components/
```

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/health` | Engine status |
| POST | `/api/command` | Send raw CLI command to engine |
| POST | `/api/simulate` | Run transient sim + return JSON waveform |
| POST | `/api/clear` | Clear circuit state |
| GET | `/api/logs/stream` | SSE stream of engine output |

## C++ Engine Commands

```
ADD_R <name> <n1> <n2> <value>
ADD_C <name> <n1> <n2> <value>
ADD_L <name> <n1> <n2> <value>
ADD_STEP_V <name> <n1> <n2> <v_start> <v_end> <t_step>
ADD_DIODE <name> <anode> <cathode>
ADD_BJT <name> <NPN|PNP> <collector> <base> <emitter>
ADD_MOSFET <name> <NMOS|PMOS> <drain> <gate> <source>
ADD_GATE <name> <type> <out> <in1> [in2] <v_high> <v_low>
SIM <t1> <t2> <dt>
OSC_JSON <node>
OSC_PPM <filename> <node>
CLEAR
EXIT
```

## Example: RC Circuit via CLI

```
ADD_STEP_V V1 1 0 0 5 1m
ADD_R R1 1 2 1k
ADD_C C1 2 0 100u
SIM 0 20m 10u
OSC_JSON 2
```

## Legacy Python GUI

The original Tkinter interface is still available:

```bash
pip install pillow
python app.py
```
