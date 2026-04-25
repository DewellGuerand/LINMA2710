// nn_data_parallel.cpp
// Data-parallel neural network training using MPI.
//
// Alternative approach to DistributedMatrix (column partition):
//   - Weights W1, W2 are REPLICATED on every process
//   - Training data is PARTITIONED row-wise (each process handles N/P samples)
//   - Each process computes local gradients from its data partition
//   - MPI_Allreduce synchronizes gradients once per epoch
//   - All processes update weights identically => no divergence
//
// Compare with DistributedMatrix::multiplyTransposed:
//   - There, communication volume = output matrix size (m² doubles)
//   - Here,  communication volume = gradient size (D*H + H*K doubles, fixed)

#include "matrix.hpp"
#include <mpi.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

// ---- Hyperparameters ----
constexpr int N      = 4096;  // total training samples (must be divisible by P)
constexpr int D      = 64;    // input features
constexpr int H      = 32;    // hidden units
constexpr int K      = 1;     // output units
constexpr int EPOCHS = 100;
constexpr double LR  = 0.01;

// ---- Helper: MPI_Allreduce on a Matrix (sum across all processes) ----
// Uses a local buffer since Matrix::data is private.
void allreduce_matrix(Matrix& m) {
    int n = m.numRows() * m.numCols();
    std::vector<double> buf(n);
    for (int i = 0; i < m.numRows(); i++)
        for (int j = 0; j < m.numCols(); j++)
            buf[i * m.numCols() + j] = m.get(i, j);
    MPI_Allreduce(MPI_IN_PLACE, buf.data(), n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    for (int i = 0; i < m.numRows(); i++)
        for (int j = 0; j < m.numCols(); j++)
            m.set(i, j, buf[i * m.numCols() + j]);
}

// ---- Data generation (deterministic, no file I/O) ----
// Each process generates only its own rows using the global sample index g.
// Target: mean of all input features.
void generate_data(Matrix& X, Matrix& Y, int global_start) {
    for (int i = 0; i < X.numRows(); i++) {
        int g = global_start + i;
        double target = 0.0;
        for (int j = 0; j < D; j++) {
            double val = std::sin(2.0 * M_PI * (g * D + j) / (N * D));
            X.set(i, j, val);
            target += val;
        }
        Y.set(i, 0, target / D);
    }
}

// ---- Weight initialization (deterministic, identical on all processes) ----
// All processes compute the same weights => no broadcast needed.
void init_weights(Matrix& W, double scale, int seed) {
    for (int i = 0; i < W.numRows(); i++)
        for (int j = 0; j < W.numCols(); j++)
            W.set(i, j, scale * std::sin((double)(seed + i * W.numCols() + j)));
}

// ---- Element-wise product of three matrices: result[i,j] = a[i,j]*b[i,j]*c[i,j] ----
Matrix hadamard3(const Matrix& a, const Matrix& b, const Matrix& c) {
    Matrix result(a.numRows(), a.numCols());
    for (int i = 0; i < a.numRows(); i++)
        for (int j = 0; j < a.numCols(); j++)
            result.set(i, j, a.get(i,j) * b.get(i,j) * c.get(i,j));
    return result;
}

// ---- Scalar fill helper ----
Matrix ones_like(const Matrix& m) {
    Matrix result(m.numRows(), m.numCols());
    result.fill(1.0);
    return result;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, P;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    if (N % P != 0) {
        if (rank == 0)
            std::cerr << "Error: N=" << N << " must be divisible by P=" << P << "\n";
        MPI_Finalize();
        return 1;
    }

    const int local_N      = N / P;
    const int global_start = rank * local_N;

    // ---- Allocate data matrices (local partition) ----
    Matrix X_local(local_N, D);
    Matrix Y_local(local_N, K);
    generate_data(X_local, Y_local, global_start);

    // ---- Initialize weights (replicated on all processes) ----
    Matrix W1(D, H);
    Matrix W2(H, K);
    init_weights(W1, 0.1, 0);
    init_weights(W2, 0.1, D * H);

    // ---- CSV header (rank 0 only) ----
    if (rank == 0)
        std::cout << "epoch,loss,compute_ms,comm_ms\n";

    double total_compute = 0.0, total_comm = 0.0;

    auto sigmoid_fn = [](double x) {
        return x >= 0.0 ? 1.0 / (1.0 + std::exp(-x))
                        : std::exp(x) / (1.0 + std::exp(x));
    };

    // ---- Training loop ----
    for (int epoch = 0; epoch < EPOCHS; epoch++) {

        // -- Compute phase: forward + backward pass --
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        // Forward pass
        Matrix Z1   = X_local * W1;                  // [local_N x H]
        Matrix H_act = Z1.apply(sigmoid_fn);          // [local_N x H]
        Matrix Y_hat = H_act * W2;                    // [local_N x K]  (linear output)

        // MSE loss gradient at output layer
        Matrix dZ2 = Y_hat - Y_local;                 // [local_N x K]
        double loss_local = 0.0;
        for (int i = 0; i < local_N; i++)
            for (int j = 0; j < K; j++) {
                double e = dZ2.get(i, j);
                loss_local += 0.5 * e * e;
            }

        // Backward pass
        Matrix dW2_local = H_act.transpose() * dZ2;  // [H x K]

        Matrix dH   = dZ2 * W2.transpose();           // [local_N x H]
        // sigmoid'(z) = h*(1-h), so dZ1 = dH .* H_act .* (1 - H_act)
        Matrix one_minus_H = ones_like(H_act) - H_act;
        Matrix dZ1  = hadamard3(dH, H_act, one_minus_H); // [local_N x H]

        Matrix dW1_local = X_local.transpose() * dZ1; // [D x H]

        MPI_Barrier(MPI_COMM_WORLD);
        double t1 = MPI_Wtime();
        double compute_ms = (t1 - t0) * 1000.0;
        total_compute += compute_ms;

        // -- Communication phase: synchronize gradients --
        double t2 = MPI_Wtime();

        allreduce_matrix(dW1_local);
        allreduce_matrix(dW2_local);
        MPI_Allreduce(MPI_IN_PLACE, &loss_local, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        double t3 = MPI_Wtime();
        double comm_ms = (t3 - t2) * 1000.0;
        total_comm += comm_ms;

        // Scale gradients to mean over all N samples (each local gradient
        // summed over local_N samples; AllReduce summed P contributions)
        dW1_local = dW1_local * (1.0 / N);
        dW2_local = dW2_local * (1.0 / N);
        double loss_global = loss_local / N;

        // Weight update (identical on all processes)
        W1.sub_mul(LR, dW1_local);
        W2.sub_mul(LR, dW2_local);

        if (rank == 0 && (epoch % 10 == 0 || epoch == EPOCHS - 1))
            std::cout << epoch << "," << std::fixed << std::setprecision(6)
                      << loss_global << "," << compute_ms << "," << comm_ms << "\n";
    }

    if (rank == 0) {
        double comm_ratio = total_comm / (total_compute + total_comm) * 100.0;
        std::cerr << "# nprocs=" << P
                  << " total_compute_ms=" << total_compute
                  << " total_comm_ms="    << total_comm
                  << " comm_fraction="    << std::fixed << std::setprecision(1)
                  << comm_ratio << "%\n";
    }

    MPI_Finalize();
    return 0;
}
