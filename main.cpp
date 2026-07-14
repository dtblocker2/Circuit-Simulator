#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <sstream>

// ==========================================
// 1. LINEAR EQUATION SOLVER
// ==========================================
class LinearSolver {
public:
    static std::vector<double> solve(std::vector<std::vector<double>> A, std::vector<double> b) {
        int n = A.size();
        for (int i = 0; i < n; i++) {
            int maxRow = i;
            for (int k = i + 1; k < n; k++) {
                if (std::abs(A[k][i]) > std::abs(A[maxRow][i])) maxRow = k;
            }
            if (std::abs(A[maxRow][i]) < 1e-12) {
                throw std::runtime_error("Matrix is singular: Floating nodes or shorted sources detected.");
            }
            std::swap(A[i], A[maxRow]);
            std::swap(b[i], b[maxRow]);

            for (int k = i + 1; k < n; k++) {
                double factor = A[k][i] / A[i][i];
                for (int j = i; j < n; j++) A[k][j] -= factor * A[i][j];
                b[k] -= factor * b[i];
            }
        }
        std::vector<double> x(n, 0.0);
        for (int i = n - 1; i >= 0; i--) {
            double sum = 0.0;
            for (int j = i + 1; j < n; j++) sum += A[i][j] * x[j];
            x[i] = (b[i] - sum) / A[i][i];
        }
        return x;
    }
};

// ==========================================
// 2. COMPONENT OOP HIERARCHY
// ==========================================
class Component {
protected:
    std::string name;
    int node1, node2;
public:
    Component(std::string n, int n1, int n2) : name(std::move(n)), node1(n1), node2(n2) {}
    virtual ~Component() = default;
    virtual std::string getType() const = 0;
    virtual double getValue() const = 0;
    std::string getName() const { return name; }
    int getNode1() const { return node1; }
    int getNode2() const { return node2; }
};

class Resistor : public Component {
    double resistance;
public:
    Resistor(std::string n, int n1, int n2, double r) : Component(std::move(n), n1, n2), resistance(r) {
        if (r <= 0.0) throw std::invalid_argument("Resistance must be > 0.");
    }
    std::string getType() const override { return "Resistor"; }
    double getValue() const override { return resistance; }
};

class VoltageSource : public Component {
    double voltage;
public:
    VoltageSource(std::string n, int n1, int n2, double v) : Component(std::move(n), n1, n2), voltage(v) {}
    std::string getType() const override { return "VoltageSource"; }
    double getValue() const override { return voltage; }
};

// ==========================================
// 3. CIRCUIT ENGINE (WITH DELETION SUPPORT)
// ==========================================
class Circuit {
private:
    std::vector<std::unique_ptr<Component>> components;
    std::vector<VoltageSource*> voltageSources;
    std::vector<Resistor*> resistors;
    std::map<std::string, int> vsIndexMap;
    std::vector<double> solution;
    int maxNodeId = 0;
    bool isSolved = false;

    void analyzeNodes() {
        maxNodeId = 0;
        voltageSources.clear();
        resistors.clear();
        vsIndexMap.clear();
        for (const auto& comp : components) {
            maxNodeId = std::max({maxNodeId, comp->getNode1(), comp->getNode2()});
            if (comp->getType() == "VoltageSource") {
                vsIndexMap[comp->getName()] = voltageSources.size();
                voltageSources.push_back(static_cast<VoltageSource*>(comp.get()));
            } else if (comp->getType() == "Resistor") {
                resistors.push_back(static_cast<Resistor*>(comp.get()));
            }
        }
    }

public:
    void addResistor(const std::string& name, int n1, int n2, double r) {
        removeComponent(name); // Overwrite if name already exists
        components.push_back(std::make_unique<Resistor>(name, n1, n2, r));
        isSolved = false;
    }

    void addVoltageSource(const std::string& name, int n1, int n2, double v) {
        removeComponent(name); // Overwrite if name already exists
        components.push_back(std::make_unique<VoltageSource>(name, n1, n2, v));
        isSolved = false;
    }

    bool removeComponent(const std::string& name) {
        auto it = std::remove_if(components.begin(), components.end(),
            [&name](const std::unique_ptr<Component>& comp) {
                return comp->getName() == name;
            });
        if (it != components.end()) {
            components.erase(it, components.end());
            isSolved = false;
            return true;
        }
        return false;
    }

    void clear() {
        components.clear();
        isSolved = false;
    }

    void solve() {
        analyzeNodes();
        int N = maxNodeId;
        int M = voltageSources.size();
        int matrixSize = N + M;

        if (N == 0) throw std::runtime_error("Circuit is empty.");

        std::vector<std::vector<double>> A(matrixSize, std::vector<double>(matrixSize, 0.0));
        std::vector<double> Z(matrixSize, 0.0);

        for (const auto& res : resistors) {
            double g = 1.0 / res->getValue();
            int n1 = res->getNode1(), n2 = res->getNode2();
            if (n1 > 0) A[n1 - 1][n1 - 1] += g;
            if (n2 > 0) A[n2 - 1][n2 - 1] += g;
            if (n1 > 0 && n2 > 0) {
                A[n1 - 1][n2 - 1] -= g;
                A[n2 - 1][n1 - 1] -= g;
            }
        }

        for (int i = 0; i < M; i++) {
            int vsCol = N + i;
            int n1 = voltageSources[i]->getNode1(), n2 = voltageSources[i]->getNode2();
            if (n1 > 0) { A[n1 - 1][vsCol] = 1.0; A[vsCol][n1 - 1] = 1.0; }
            if (n2 > 0) { A[n2 - 1][vsCol] = -1.0; A[vsCol][n2 - 1] = -1.0; }
            Z[vsCol] = voltageSources[i]->getValue();
        }

        solution = LinearSolver::solve(A, Z);
        isSolved = true;
    }

    double getNodeVoltage(int node) const {
        if (!isSolved) throw std::runtime_error("Simulation not run. Command: SIMULATE first.");
        if (node == 0) return 0.0;
        if (node < 0 || node > maxNodeId) throw std::out_of_range("Node ID does not exist.");
        return solution[node - 1];
    }

    double getResistorCurrent(const std::string& name) const {
        if (!isSolved) throw std::runtime_error("Simulation not run. Command: SIMULATE first.");
        for (const auto& res : resistors) {
            if (res->getName() == name) {
                return (getNodeVoltage(res->getNode1()) - getNodeVoltage(res->getNode2())) / res->getValue();
            }
        }
        throw std::invalid_argument("Resistor not found: " + name);
    }
    
    int getComponentCount() const { return components.size(); }
};

// ==========================================
// 4. GUI INTERFACE COMMAND LOOP
// ==========================================
void runCommandLoop(Circuit& circuit) {
    std::string line;
    std::cout << "[STATUS] READY\n";

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        // Convert command to uppercase for case-insensitivity
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        try {
            if (cmd == "ADD_R") {
                std::string name;
                int n1, n2;
                double val;
                if (!(iss >> name >> n1 >> n2 >> val)) throw std::invalid_argument("Usage: ADD_R <name> <node1> <node2> <val>");
                circuit.addResistor(name, n1, n2, val);
                std::cout << "[OK] Added Resistor " << name << " (" << val << " Ohm)\n";
            } 
            else if (cmd == "ADD_V") {
                std::string name;
                int n1, n2;
                double val;
                if (!(iss >> name >> n1 >> n2 >> val)) throw std::invalid_argument("Usage: ADD_V <name> <node1> <node2> <val>");
                circuit.addVoltageSource(name, n1, n2, val);
                std::cout << "[OK] Added Voltage Source " << name << " (" << val << " V)\n";
            } 
            else if (cmd == "DEL" || cmd == "DELETE") {
                std::string name;
                if (!(iss >> name)) throw std::invalid_argument("Usage: DEL <component_name>");
                if (circuit.removeComponent(name)) {
                    std::cout << "[OK] Deleted component: " << name << "\n";
                } else {
                    std::cout << "[ERROR] Component not found: " << name << "\n";
                }
            } 
            else if (cmd == "SIMULATE" || cmd == "SIM") {
                circuit.solve();
                std::cout << "[OK] Simulation converged successfully.\n";
            } 
            else if (cmd == "GET_V") {
                int node;
                if (!(iss >> node)) throw std::invalid_argument("Usage: GET_V <node_id>");
                double v = circuit.getNodeVoltage(node);
                std::cout << "[RESULT] V(" << node << ") = " << std::fixed << std::setprecision(4) << v << "\n";
            } 
            else if (cmd == "GET_I") {
                std::string name;
                if (!(iss >> name)) throw std::invalid_argument("Usage: GET_I <resistor_name>");
                double i = circuit.getResistorCurrent(name);
                std::cout << "[RESULT] I(" << name << ") = " << std::fixed << std::setprecision(4) << i << "\n";
            } 
            else if (cmd == "CLEAR") {
                circuit.clear();
                std::cout << "[OK] Circuit cleared.\n";
            } 
            else if (cmd == "EXIT" || cmd == "QUIT") {
                std::cout << "[STATUS] EXITING\n";
                break;
            } 
            else {
                std::cout << "[ERROR] Unknown command: " << cmd << "\n";
            }
        } 
        catch (const std::exception& e) {
            // Catch all mathematical, parsing, or bounds errors so the loop never crashes
            std::cout << "[ERROR] " << e.what() << "\n";
        }

        // Flush output stream immediately so the GUI receives data without buffering delay
        std::cout << std::flush;
    }
}

int main() {
    Circuit circuit;
    runCommandLoop(circuit);
    return 0;
}