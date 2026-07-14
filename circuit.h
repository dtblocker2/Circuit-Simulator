#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "components.h"
#include "solver.h"
#include "oscilloscope.h"
#include <vector>
#include <memory>
#include <iostream>

class Circuit : public Stamper {
    std::vector<std::unique_ptr<Component>> components;
    std::vector<std::vector<double>> A;
    std::vector<double> Z;
    int maxNode = 0;
    Oscilloscope osc;

    void analyzeNodes() {
        maxNode = 0;
        for (const auto& c : components) {
            for (int n : c->getNodes()) maxNode = std::max(maxNode, n);
        }
    }

public:
    void addComponent(std::unique_ptr<Component> c) { components.push_back(std::move(c)); }
    void clear() { components.clear(); osc.clear(); }
    const Oscilloscope& getOscilloscope() const { return osc; }

    void addConductance(int n1, int n2, double g) override {
        if (n1 > 0) A[n1-1][n1-1] += g;
        if (n2 > 0) A[n2-1][n2-1] += g;
        if (n1 > 0 && n2 > 0) { A[n1-1][n2-1] -= g; A[n2-1][n1-1] -= g; }
    }
    void addTransconductance(int outP, int outN, int inP, int inN, double gm) override {
        if (outP > 0 && inP > 0) A[outP-1][inP-1] += gm;
        if (outP > 0 && inN > 0) A[outP-1][inN-1] -= gm;
        if (outN > 0 && inP > 0) A[outN-1][inP-1] -= gm;
        if (outN > 0 && inN > 0) A[outN-1][inN-1] += gm;
    }
    void addCurrentSource(int n1, int n2, double i) override {
        if (n1 > 0) Z[n1-1] -= i; // Leaving n1
        if (n2 > 0) Z[n2-1] += i; // Entering n2
    }

    void simulateTransient(double t1, double t2, double dt) {
        analyzeNodes();
        if (maxNode == 0) throw std::runtime_error("Empty circuit.");
        osc.clear();

        int steps = static_cast<int>(t2 / dt);
        std::vector<double> v_sol(maxNode, 0.0);

        for (int s = 0; s <= steps; s++) {
            double time = s * dt;
            
            // Newton-Raphson Loop for non-linear convergence
            for (int iter = 0; iter < 15; iter++) {
                A.assign(maxNode, std::vector<double>(maxNode, 0.0));
                Z.assign(maxNode, 0.0);

                for (const auto& c : components) {
                    c->stampLinear(*this, dt, time);
                    c->stampNonLinear(*this, v_sol);
                }
                std::vector<double> v_new = LinearSolver::solve(A, Z);
                
                double max_diff = 0.0;
                for (int i = 0; i < maxNode; i++) max_diff = std::max(max_diff, std::abs(v_new[i] - v_sol[i]));
                v_sol = v_new;
                if (max_diff < 1e-6) break;
            }
            for (auto& c : components) c->updateHistory(v_sol, dt);
            if (time >= t1 - dt*0.5) osc.record(time, v_sol);
        }
        std::cout << "[OK] Transient simulation converged successfully.\n";
    }
};

#endif // CIRCUIT_H