import sys
import os
import subprocess
import threading
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image, ImageTk

class CircuitGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("MNA Circuit Studio — Component Builder & Interactive Oscilloscope")
        self.root.geometry("1180x740")
        self.root.minsize(1000, 600)
        
        # Locate C++ engine executable
        exe = "./sim_engine" if os.name != 'nt' else "sim_engine.exe"
        if not os.path.exists(exe):
            messagebox.showerror("Engine Missing", 
                "Please compile the C++ backend first using:\n"
                "g++ -std=c++17 -O3 main.cpp -o sim_engine")
            sys.exit(1)

        self.proc = subprocess.Popen(
            [exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
            text=True, bufsize=1
        )
        self.meta = {} # Stores oscilloscope coordinate metadata from C++
        
        self.setup_ui()
        threading.Thread(target=self.read_engine, daemon=True).start()

    def setup_ui(self):
        # Master Layout: Left Control Panel (Tabs) + Right Canvas Display
        left_pane = tk.Frame(self.root, width=420, padx=10, pady=10)
        left_pane.pack(side=tk.LEFT, fill=tk.Y)
        left_pane.pack_propagate(False) # Prevent shrinking
        
        right_pane = tk.Frame(self.root, padx=10, pady=10)
        right_pane.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # ---- TABS FOR CLEAN LAYOUT ----
        self.tabs = ttk.Notebook(left_pane)
        self.tabs.pack(fill=tk.BOTH, expand=True)

        tab_builder = ttk.Frame(self.tabs, padding=10)
        tab_console = ttk.Frame(self.tabs, padding=10)
        self.tabs.add(tab_builder, text=" 🛠️ Circuit Builder ")
        self.tabs.add(tab_console, text=" 💻 CLI & Logs ")

        self.build_builder_tab(tab_builder)
        self.build_console_tab(tab_console)

        # ---- RIGHT PANE: INTERACTIVE OSCILLOSCOPE ----
        self.cursor_lbl = tk.Label(right_pane, text="Hover over graph to inspect coordinates", font=("Courier", 11, "bold"), fg="#00AA00")
        self.cursor_lbl.pack(anchor=tk.W, pady=(0, 5))
        
        self.canvas = tk.Canvas(right_pane, bg="black", width=720, height=540)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Motion>", self.on_mouse_move)

    # ==========================================
    # TAB 1: FORM-BASED COMPONENT BUILDER
    # ==========================================
    def build_builder_tab(self, parent):
        tk.Label(parent, text="1. Add Circuit Component", font=("Arial", 11, "bold"), fg="#333").pack(anchor=tk.W, pady=(0, 5))
        
        form = ttk.LabelFrame(parent, text=" Component Specification ", padding=10)
        form.pack(fill=tk.X, pady=5)

        # Component Type Dropdown
        tk.Label(form, text="Component Type:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.type_var = tk.StringVar(value="Resistor (R)")
        types = [
            "Resistor (R)", "Capacitor (C)", "Inductor (L)", 
            "Diode (D)", "BJT Transistor", "MOSFET Transistor", 
            "Logic Gate", "Step Voltage Source"
        ]
        self.type_cb = ttk.Combobox(form, textvariable=self.type_var, values=types, state="readonly")
        self.type_cb.grid(row=0, column=1, sticky=tk.EW, pady=2)
        self.type_cb.bind("<<ComboboxSelected>>", self.on_type_change)

        # Name Entry
        tk.Label(form, text="Unique Name:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.ent_name = ttk.Entry(form)
        self.ent_name.insert(0, "R1")
        self.ent_name.grid(row=1, column=1, sticky=tk.EW, pady=2)

        # Dynamic Node Labels
        self.lbl_n1 = tk.Label(form, text="Node 1 (+):")
        self.lbl_n1.grid(row=2, column=0, sticky=tk.W, pady=2)
        self.ent_n1 = ttk.Entry(form)
        self.ent_n1.insert(0, "1")
        self.ent_n1.grid(row=2, column=1, sticky=tk.EW, pady=2)

        self.lbl_n2 = tk.Label(form, text="Node 2 (-/Gnd):")
        self.lbl_n2.grid(row=3, column=0, sticky=tk.W, pady=2)
        self.ent_n2 = ttk.Entry(form)
        self.ent_n2.insert(0, "0")
        self.ent_n2.grid(row=3, column=1, sticky=tk.EW, pady=2)

        self.lbl_n3 = tk.Label(form, text="Node 3 (Unused):")
        self.lbl_n3.grid(row=4, column=0, sticky=tk.W, pady=2)
        self.ent_n3 = ttk.Entry(form, state="disabled")
        self.ent_n3.grid(row=4, column=1, sticky=tk.EW, pady=2)

        # Value / Parameter Entry
        self.lbl_val = tk.Label(form, text="Value (e.g. 10k, 100u):")
        self.lbl_val.grid(row=5, column=0, sticky=tk.W, pady=2)
        self.ent_val = ttk.Entry(form)
        self.ent_val.insert(0, "1k")
        self.ent_val.grid(row=5, column=1, sticky=tk.EW, pady=2)

        form.columnconfigure(1, weight=1)

        ttk.Button(parent, text="➕ Add Component to Circuit", command=self.add_component_from_form).pack(fill=tk.X, pady=(5, 15))

        # ---- SECTION 2: TRANSIENT SIMULATION CONTROLS ----
        tk.Label(parent, text="2. Simulate & Generate Waveform", font=("Arial", 11, "bold"), fg="#333").pack(anchor=tk.W, pady=(5, 5))
        
        sim_box = ttk.LabelFrame(parent, text=" Time-Domain Parameters ", padding=10)
        sim_box.pack(fill=tk.X, pady=5)

        # t1, t2, dt Grid
        tk.Label(sim_box, text="Start Time (t1):").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.ent_t1 = ttk.Entry(sim_box, width=10)
        self.ent_t1.insert(0, "0")
        self.ent_t1.grid(row=0, column=1, sticky=tk.W, pady=2)

        tk.Label(sim_box, text="End Time (t2):").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.ent_t2 = ttk.Entry(sim_box, width=10)
        self.ent_t2.insert(0, "10m")
        self.ent_t2.grid(row=1, column=1, sticky=tk.W, pady=2)

        tk.Label(sim_box, text="Time Step (dt):").grid(row=2, column=0, sticky=tk.W, pady=2)
        self.ent_dt = ttk.Entry(sim_box, width=10)
        self.ent_dt.insert(0, "10u")
        self.ent_dt.grid(row=2, column=1, sticky=tk.W, pady=2)

        tk.Label(sim_box, text="Target Node ID:").grid(row=3, column=0, sticky=tk.W, pady=2)
        self.ent_node = ttk.Entry(sim_box, width=10)
        self.ent_node.insert(0, "1")
        self.ent_node.grid(row=3, column=1, sticky=tk.W, pady=2)

        ttk.Button(parent, text="▶ Run Simulation & Plot Graph", command=self.run_sim_and_plot).pack(fill=tk.X, pady=10)
        ttk.Button(parent, text="🗑️ Clear All Circuit Memory", command=lambda: self.send("CLEAR\n")).pack(fill=tk.X, pady=2)

    def on_type_change(self, event=None):
        """Dynamically updates field labels depending on the chosen component."""
        c_type = self.type_var.get()
        self.ent_n3.config(state="normal")
        
        if c_type in ["Resistor (R)", "Capacitor (C)", "Inductor (L)"]:
            self.lbl_n1.config(text="Node 1 (+):"); self.lbl_n2.config(text="Node 2 (-):")
            self.lbl_n3.config(text="Unused:"); self.ent_n3.config(state="disabled")
            self.lbl_val.config(text="Value (SI: k, u, n, Meg):")
        elif c_type == "Diode (D)":
            self.lbl_n1.config(text="Anode (+):"); self.lbl_n2.config(text="Cathode (-):")
            self.lbl_n3.config(text="Unused:"); self.ent_n3.config(state="disabled")
            self.lbl_val.config(text="Unused:"); self.ent_val.delete(0, tk.END); self.ent_val.insert(0, "1")
        elif c_type == "BJT Transistor":
            self.lbl_n1.config(text="Collector:"); self.lbl_n2.config(text="Base:")
            self.lbl_n3.config(text="Emitter:")
            self.lbl_val.config(text="Type (NPN / PNP):"); self.ent_val.delete(0, tk.END); self.ent_val.insert(0, "NPN")
        elif c_type == "MOSFET Transistor":
            self.lbl_n1.config(text="Drain:"); self.lbl_n2.config(text="Gate:")
            self.lbl_n3.config(text="Source:")
            self.lbl_val.config(text="Type (NMOS / PMOS):"); self.ent_val.delete(0, tk.END); self.ent_val.insert(0, "NMOS")
        elif c_type == "Logic Gate":
            self.lbl_n1.config(text="Output Node:"); self.lbl_n2.config(text="Input 1:")
            self.lbl_n3.config(text="Input 2 (0 if NOT):")
            self.lbl_val.config(text="Type Vhigh Vlow (e.g. NAND 5 0):"); self.ent_val.delete(0, tk.END); self.ent_val.insert(0, "NOT 5 0")
        elif c_type == "Step Voltage Source":
            self.lbl_n1.config(text="Node (+) :"); self.lbl_n2.config(text="Node (-) :");
            self.lbl_n3.config(text="Unused:"); self.ent_n3.config(state="disabled")
            self.lbl_val.config(text="Vstart Vend Tstep (e.g. 0 5 1m):"); self.ent_val.delete(0, tk.END); self.ent_val.insert(0, "0 5 1m")

    def add_component_from_form(self):
        """Translates GUI inputs into structured CLI commands for the C++ engine."""
        c_type = self.type_var.get()
        name = self.ent_name.get().strip()
        n1 = self.ent_n1.get().strip()
        n2 = self.ent_n2.get().strip()
        n3 = self.ent_n3.get().strip()
        val = self.ent_val.get().strip()

        if not name or not n1 or not n2:
            messagebox.showwarning("Incomplete Form", "Please provide a Component Name and at least two Node IDs.")
            return

        cmd = ""
        if c_type == "Resistor (R)": cmd = f"ADD_R {name} {n1} {n2} {val}\n"
        elif c_type == "Capacitor (C)": cmd = f"ADD_C {name} {n1} {n2} {val}\n"
        elif c_type == "Inductor (L)": cmd = f"ADD_L {name} {n1} {n2} {val}\n"
        elif c_type == "Diode (D)": cmd = f"ADD_DIODE {name} {n1} {n2}\n"
        elif c_type == "BJT Transistor": cmd = f"ADD_BJT {name} {val} {n1} {n2} {n3}\n"
        elif c_type == "MOSFET Transistor": cmd = f"ADD_MOSFET {name} {val} {n1} {n2} {n3}\n"
        elif c_type == "Logic Gate":
            parts = val.split()
            if len(parts) < 3:
                messagebox.showerror("Format Error", "Logic Gate value must be: TYPE V_HIGH V_LOW (e.g., 'NAND 5.0 0.0')")
                return
            g_type, vh, vl = parts[0], parts[1], parts[2]
            cmd = f"ADD_GATE {name} {g_type} {n1} {n2} " + (f"{n3} " if g_type != "NOT" else "") + f"{vh} {vl}\n"
        elif c_type == "Step Voltage Source":
            parts = val.split()
            if len(parts) < 3:
                messagebox.showerror("Format Error", "Step Voltage value must be: V_START V_END T_STEP (e.g., '0 12 1m')")
                return
            cmd = f"ADD_STEP_V {name} {n1} {n2} {parts[0]} {parts[1]} {parts[2]}\n"

        self.send(cmd)

    def run_sim_and_plot(self):
        """Triggers the time-domain simulator from t1 to t2 and plots the result."""
        t1, t2, dt = self.ent_t1.get().strip(), self.ent_t2.get().strip(), self.ent_dt.get().strip()
        node = self.ent_node.get().strip()
        
        self.send(f"SIM {t1} {t2} {dt}\n")
        self.send(f"OSC_PPM trace.ppm {node}\n")

    # ==========================================
    # TAB 2: CLI TERMINAL & ENGINE LOGS
    # ==========================================
    def build_console_tab(self, parent):
        tk.Label(parent, text="Pre-Built Test Macros:", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=2)
        
        macros = [
            ("Load CMOS Inverter Demo", "CLEAR\nADD_GATE G1 NOT 2 1 5 0\nADD_STEP_V V1 1 0 0 5 2u\nSIM 0 10u 10n\nOSC_PPM trace.ppm 2\n"),
            ("Load Diode Rectifier Demo", "CLEAR\nADD_STEP_V V1 1 0 -5 5 2m\nADD_DIODE D1 1 2\nADD_R R1 2 0 1k\nSIM 0 5m 5u\nOSC_PPM trace.ppm 2\n")
        ]
        for lbl, cmd in macros:
            ttk.Button(parent, text=lbl, command=lambda c=cmd: self.send(c)).pack(fill=tk.X, pady=2)

        tk.Label(parent, text="Manual Command Terminal:", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(15, 2))
        self.cli_entry = ttk.Entry(parent, font=("Courier", 10))
        self.cli_entry.pack(fill=tk.X, pady=2)
        self.cli_entry.bind("<Return>", lambda e: (self.send(self.cli_entry.get() + "\n"), self.cli_entry.delete(0, tk.END)))
        ttk.Button(parent, text="Execute Raw Command", command=lambda: (self.send(self.cli_entry.get() + "\n"), self.cli_entry.delete(0, tk.END))).pack(fill=tk.X, pady=2)

        tk.Label(parent, text="Backend Engine Logs:", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(10, 2))
        self.log = tk.Text(parent, width=45, height=18, font=("Courier", 9))
        self.log.pack(fill=tk.BOTH, expand=True)

        ttk.Button(parent, text="Exit Application", command=self.close).pack(fill=tk.X, pady=(10, 0))

    # ==========================================
    # COMMUNICATION & CANVAS RENDERING
    # ==========================================
    def send(self, cmd):
        if not cmd.endswith('\n'): cmd += '\n'
        self.proc.stdin.write(cmd)
        self.proc.stdin.flush()

    def read_engine(self):
        while line := self.proc.stdout.readline():
            self.root.after(0, self.log.insert, tk.END, line)
            self.root.after(0, self.log.see, tk.END)
            if "[OSC_META]" in line:
                self.parse_meta(line)
            elif "[OSC] Picture generated successfully:" in line:
                fn = line.split(":")[1].split("(")[0].strip()
                self.root.after(50, self.load_image, fn)

    def parse_meta(self, line):
        parts = line.replace("[OSC_META]", "").strip().split()
        for p in parts:
            if "=" in p:
                k, v = p.split("=")
                self.meta[k] = float(v) if k != "unit" else v

    def load_image(self, fn):
        if os.path.exists(fn):
            img = Image.open(fn).resize((720, 540), Image.Resampling.LANCZOS)
            self.photo = ImageTk.PhotoImage(img)
            self.canvas.delete("all")
            self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)

    def on_mouse_move(self, event):
        if not self.meta or "imgW" not in self.meta: return
        scale_x = self.meta["imgW"] / 720.0
        scale_y = self.meta["imgH"] / 540.0
        mx, my = event.x * scale_x, event.y * scale_y

        padL, padR, padT, padB = self.meta["padL"], self.meta["padR"], self.meta["padT"], self.meta["padB"]
        plotW, plotH = self.meta["imgW"] - padL - padR, self.meta["imgH"] - padT - padB

        if padL <= mx <= padL + plotW and padT <= my <= padT + plotH:
            norm_x = (mx - padL) / plotW
            norm_y = (padT + plotH - my) / plotH
            
            t = self.meta["t_start"] + norm_x * (self.meta["t_end"] - self.meta["t_start"])
            v = self.meta["y_min"] + norm_y * (self.meta["y_max"] - self.meta["y_min"])
            
            self.cursor_lbl.config(text=f"Time: {t * 1e3:.3f} ms | Voltage: {v:.4f} V")
            self.canvas.delete("cursor")
            self.canvas.create_line(event.x, 0, event.x, 540, fill="#FF4444", dash=(4,4), tags="cursor")
            self.canvas.create_line(0, event.y, 720, event.y, fill="#FF4444", dash=(4,4), tags="cursor")
        else:
            self.canvas.delete("cursor")
            self.cursor_lbl.config(text="Hover over graph to inspect coordinates")

    def close(self):
        self.send("EXIT\n")
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = CircuitGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.close)
    root.mainloop()