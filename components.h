#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct Stamper {
    virtual void addConductance(int n1, int n2, double g) = 0;
    virtual void addTransconductance(int outPos, int outNeg, int inPos, int inNeg, double gm) = 0;
    virtual void addCurrentSource(int n1, int n2, double i) = 0;
};

class Component {
protected:
    std::string name;
    std::vector<int> nodes;
public:
    Component(std::string n, std::vector<int> nds) : name(std::move(n)), nodes(std::move(nds)) {}
    virtual ~Component() = default;
    virtual std::string getType() const = 0;
    virtual void stampLinear(Stamper& st, double dt, double t) {}
    virtual void stampNonLinear(Stamper& st, const std::vector<double>& v_guess) {}
    virtual void updateHistory(const std::vector<double>& v_sol, double dt) {}
    std::string getName() const { return name; }
    const std::vector<int>& getNodes() const { return nodes; }
};

// --- PASSIVES ---
class Resistor : public Component {
    double r;
public:
    Resistor(std::string n, int n1, int n2, double val) : Component(std::move(n), {n1, n2}), r(val) {}
    std::string getType() const override { return "Resistor"; }
    void stampLinear(Stamper& st, double dt, double t) override { st.addConductance(nodes[0], nodes[1], 1.0 / r); }
};

class Capacitor : public Component {
    double c, v_old = 0.0;
public:
    Capacitor(std::string n, int n1, int n2, double val) : Component(std::move(n), {n1, n2}), c(val) {}
    std::string getType() const override { return "Capacitor"; }
    void stampLinear(Stamper& st, double dt, double t) override {
        double g = c / dt;
        st.addConductance(nodes[0], nodes[1], g);
        st.addCurrentSource(nodes[0], nodes[1], g * v_old);
    }
    void updateHistory(const std::vector<double>& v, double dt) override {
        double v1 = (nodes[0] > 0) ? v[nodes[0]-1] : 0.0;
        double v2 = (nodes[1] > 0) ? v[nodes[1]-1] : 0.0;
        v_old = v1 - v2;
    }
};

class Inductor : public Component {
    double l, i_old = 0.0;
public:
    Inductor(std::string n, int n1, int n2, double val) : Component(std::move(n), {n1, n2}), l(val) {}
    std::string getType() const override { return "Inductor"; }
    void stampLinear(Stamper& st, double dt, double t) override {
        double g = dt / l;
        st.addConductance(nodes[0], nodes[1], g);
        st.addCurrentSource(nodes[1], nodes[0], i_old);
    }
    void updateHistory(const std::vector<double>& v, double dt) override {
        double v1 = (nodes[0] > 0) ? v[nodes[0]-1] : 0.0;
        double v2 = (nodes[1] > 0) ? v[nodes[1]-1] : 0.0;
        i_old += (dt / l) * (v1 - v2);
    }
};

// --- SEMICONDUCTORS (Newton-Raphson Linearized) ---
class Diode : public Component {
    double Is = 1e-14, Vt = 0.02585;
public:
    Diode(std::string n, int n1, int n2) : Component(std::move(n), {n1, n2}) {}
    std::string getType() const override { return "Diode"; }
    void stampNonLinear(Stamper& st, const std::vector<double>& v) override {
        double v1 = (nodes[0] > 0) ? v[nodes[0]-1] : 0.0;
        double v2 = (nodes[1] > 0) ? v[nodes[1]-1] : 0.0;
        double Vd = std::clamp(v1 - v2, -5.0, 0.8); // Clamp to prevent NR overflow
        double Id = Is * (std::exp(Vd / Vt) - 1.0);
        double gd = (Is / Vt) * std::exp(Vd / Vt) + 1e-12;
        double Ieq = Id - gd * Vd;
        st.addConductance(nodes[0], nodes[1], gd);
        st.addCurrentSource(nodes[1], nodes[0], Ieq); // Current flows anode -> cathode
    }
};

// --- VOLTAGE SOURCES (Norton Equivalent Model) ---
class StepVoltageSource : public Component {
    double v_start, v_end, t_step;
    double g_int = 1e6; // 1 micro-ohm internal resistance for numerical stability
public:
    StepVoltageSource(std::string n, int n1, int n2, double vs, double ve, double ts)
        : Component(std::move(n), {n1, n2}), v_start(vs), v_end(ve), t_step(ts) {}
    
    std::string getType() const override { return "VoltageSource"; }
    
    void stampLinear(Stamper& st, double dt, double t) override {
        double v_now = (t >= t_step) ? v_end : v_start;
        
        // Stamp internal conductance between positive n1 and negative n2
        st.addConductance(nodes[0], nodes[1], g_int);
        
        // Norton current source pushes current from negative n2 into positive n1
        st.addCurrentSource(nodes[1], nodes[0], v_now * g_int);
    }
};

class BJT : public Component {
    bool is_npn;
    double bf = 100.0, Is = 1e-14, Vt = 0.02585;
public:
    BJT(std::string n, int nc, int nb, int ne, bool npn) : Component(std::move(n), {nc, nb, ne}), is_npn(npn) {}
    std::string getType() const override { return is_npn ? "NPN" : "PNP"; }
    void stampNonLinear(Stamper& st, const std::vector<double>& v) override {
        int c = nodes[0], b = nodes[1], e = nodes[2];
        double vc = (c > 0) ? v[c-1] : 0.0;
        double vb = (b > 0) ? v[b-1] : 0.0;
        double ve = (e > 0) ? v[e-1] : 0.0;
        
        double vbe = is_npn ? (vb - ve) : (ve - vb);
        vbe = std::clamp(vbe, -5.0, 0.8);
        double ib = (Is / bf) * (std::exp(vbe / Vt) - 1.0);
        double gbe = (Is / (bf * Vt)) * std::exp(vbe / Vt) + 1e-12;
        double gm = bf * gbe;
        double ieq_b = ib - gbe * vbe;
        double ieq_c = bf * ib - gm * vbe;

        if (is_npn) {
            st.addConductance(b, e, gbe);
            st.addTransconductance(c, e, b, e, gm);
            st.addCurrentSource(e, b, ieq_b);
            st.addCurrentSource(e, c, ieq_c);
        } else {
            st.addConductance(e, b, gbe);
            st.addTransconductance(e, c, e, b, gm);
            st.addCurrentSource(b, e, ieq_b);
            st.addCurrentSource(c, e, ieq_c);
        }
    }
};

class MOSFET : public Component {
    bool is_nmos;
    double kp = 2e-3, vth = 1.5, lam = 0.01;
public:
    MOSFET(std::string n, int nd, int ng, int ns, bool nmos) : Component(std::move(n), {nd, ng, ns}), is_nmos(nmos) {}
    std::string getType() const override { return is_nmos ? "NMOS" : "PMOS"; }
    void stampNonLinear(Stamper& st, const std::vector<double>& v) override {
        int d = nodes[0], g = nodes[1], s = nodes[2];
        double vd = (d > 0) ? v[d-1] : 0.0;
        double vg = (g > 0) ? v[g-1] : 0.0;
        double vs = (s > 0) ? v[s-1] : 0.0;

        double vgs = is_nmos ? (vg - vs) : (vs - vg);
        double vds = is_nmos ? (vd - vs) : (vs - vd);
        double id = 0.0, gm = 1e-12, gds = 1e-12;

        if (vgs > vth) {
            if (vds <= vgs - vth) { // Triode
                id = kp * (2.0 * (vgs - vth) * vds - vds * vds);
                gm = kp * 2.0 * vds;
                gds = kp * 2.0 * (vgs - vth - vds) + 1e-12;
            } else { // Saturation
                id = kp * (vgs - vth) * (vgs - vth) * (1.0 + lam * vds);
                gm = 2.0 * kp * (vgs - vth) * (1.0 + lam * vds);
                gds = kp * (vgs - vth) * (vgs - vth) * lam + 1e-12;
            }
        }
        double ieq = id - gm * vgs - gds * vds;
        if (is_nmos) {
            st.addConductance(d, s, gds);
            st.addTransconductance(d, s, g, s, gm);
            st.addCurrentSource(s, d, ieq);
        } else {
            st.addConductance(s, d, gds);
            st.addTransconductance(s, d, s, g, gm);
            st.addCurrentSource(d, s, ieq);
        }
    }
};

// --- LOGIC GATES (Smooth Continuous VCCS Model) ---
class LogicGate : public Component {
    std::string gate_type;
    double v_high, v_low, v_th;
public:
    LogicGate(std::string name, std::string type, int out, int in1, int in2, double vh, double vl)
        : Component(std::move(name), {out, in1, in2}), gate_type(std::move(type)), v_high(vh), v_low(vl), v_th((vh+vl)/0.5) {}
    std::string getType() const override { return "LogicGate"; }
    void stampNonLinear(Stamper& st, const std::vector<double>& v) override {
        int out = nodes[0], in1 = nodes[1], in2 = nodes[2];
        double v1 = (in1 > 0) ? v[in1-1] : 0.0;
        double v2 = (in2 > 0) ? v[in2-1] : 0.0;
        
        bool b1 = v1 > ((v_high + v_low) * 0.5);
        bool b2 = v2 > ((v_high + v_low) * 0.5);
        bool res = false;

        if (gate_type == "NOT") res = !b1;
        else if (gate_type == "AND") res = b1 && b2;
        else if (gate_type == "OR")  res = b1 || b2;
        else if (gate_type == "NAND") res = !(b1 && b2);
        else if (gate_type == "NOR")  res = !(b1 || b2);
        else if (gate_type == "XOR")  res = b1 != b2;

        double target_v = res ? v_high : v_low;
        // Model as 50-ohm output impedance source
        double rout = 50.0;
        st.addConductance(out, 0, 1.0 / rout);
        st.addCurrentSource(out, 0, target_v / rout);
    }
};

#endif // COMPONENTS_H