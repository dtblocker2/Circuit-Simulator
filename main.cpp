#include "circuit.h"
#include "si_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>

int main() {
    Circuit circuit;
    std::string line;
    std::cout << "[STATUS] READY\n";

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string cmd; iss >> cmd;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        try {
            if (cmd == "ADD_R") {
                std::string n, v; int n1, n2; iss >> n >> n1 >> n2 >> v;
                circuit.addComponent(std::make_unique<Resistor>(n, n1, n2, parseSI(v)));
                std::cout << "[OK] Added Resistor.\n";
            } else if (cmd == "ADD_C") {
                std::string n, v; int n1, n2; iss >> n >> n1 >> n2 >> v;
                circuit.addComponent(std::make_unique<Capacitor>(n, n1, n2, parseSI(v)));
                std::cout << "[OK] Added Capacitor.\n";
            } else if (cmd == "ADD_L") {
                std::string n, v; int n1, n2; iss >> n >> n1 >> n2 >> v;
                circuit.addComponent(std::make_unique<Inductor>(n, n1, n2, parseSI(v)));
                std::cout << "[OK] Added Inductor.\n";
            } else if (cmd == "ADD_STEP_V") {
                std::string n, vs, ve, ts; int n1, n2;
                iss >> n >> n1 >> n2 >> vs >> ve >> ts;
                circuit.addComponent(std::make_unique<StepVoltageSource>(n, n1, n2, parseSI(vs), parseSI(ve), parseSI(ts)));
                std::cout << "[OK] Added Step Voltage Source.\n";
            } else if (cmd == "ADD_DIODE") {
                std::string n; int n1, n2; iss >> n >> n1 >> n2;
                circuit.addComponent(std::make_unique<Diode>(n, n1, n2));
                std::cout << "[OK] Added Diode.\n";
            } else if (cmd == "ADD_BJT") {
                std::string n, type; int nc, nb, ne; iss >> n >> type >> nc >> nb >> ne;
                circuit.addComponent(std::make_unique<BJT>(n, nc, nb, ne, type == "NPN"));
                std::cout << "[OK] Added BJT.\n";
            } else if (cmd == "ADD_MOSFET") {
                std::string n, type; int nd, ng, ns; iss >> n >> type >> nd >> ng >> ns;
                circuit.addComponent(std::make_unique<MOSFET>(n, nd, ng, ns, type == "NMOS"));
                std::cout << "[OK] Added MOSFET.\n";
            } else if (cmd == "ADD_GATE") {
                std::string n, type, vh, vl; int out, in1, in2 = 0;
                iss >> n >> type >> out >> in1;
                if (type != "NOT") iss >> in2;
                iss >> vh >> vl;
                circuit.addComponent(std::make_unique<LogicGate>(n, type, out, in1, in2, parseSI(vh), parseSI(vl)));
                std::cout << "[OK] Added Logic Gate.\n";
            } else if (cmd == "SIM_TRANSIENT" || cmd == "SIM") {
                std::string t1, t2, dt; iss >> t1 >> t2 >> dt;
                circuit.simulateTransient(parseSI(t1), parseSI(t2), parseSI(dt));
            } else if (cmd == "OSC_PPM") {
                std::string fn; int node; iss >> fn >> node;
                circuit.getOscilloscope().generatePPM(fn, node);
            } else if (cmd == "OSC_JSON") {
                int node; iss >> node;
                circuit.getOscilloscope().exportJSON(node);
            } else if (cmd == "CLEAR") {
                circuit.clear(); std::cout << "[OK] Cleared.\n";
            } else if (cmd == "EXIT") {
                std::cout << "[STATUS] EXITING\n"; break;
            } else {
                std::cout << "[ERROR] Unknown command.\n";
            }
        } catch (const std::exception& e) {
            std::cout << "[ERROR] " << e.what() << "\n";
        }
        std::cout << std::flush;
    }
    return 0;
}