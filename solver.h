#ifndef SOLVER_H
#define SOLVER_H

#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

class LinearSolver {
public:
    static std::vector<double> solve(std::vector<std::vector<double>> A, std::vector<double> b) {
        int n = A.size();
        for (int i = 0; i < n; i++) {
            int maxRow = i;
            for (int k = i + 1; k < n; k++) {
                if (std::abs(A[k][i]) > std::abs(A[maxRow][i])) maxRow = k;
            }
            if (std::abs(A[maxRow][i]) < 1e-13) {
                throw std::runtime_error("Matrix singular: Floating node, shorted loop, or non-convergent NR step.");
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

#endif // SOLVER_H