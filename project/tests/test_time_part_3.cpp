#include "distributed_matrix.hpp"
#include "matrix.hpp"
#include <mpi.h>
#include <iostream>
#include <random>

int main(int argc, char** argv) {

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <M> <N> <R>" << std::endl;
        return 1;
    }

    int M = std::atoi(argv[1]);
    int N = std::atoi(argv[2]);
    int R = std::atoi(argv[3]);

    MPI_Init(&argc, &argv);
    int rank, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    // A(MxN), B(RxN) → A * B^T donne une matrice (MxR)
    Matrix A(M, N), B(R, N);
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            A.set(i, j, dist(rng));
    for (int i = 0; i < R; i++)
        for (int j = 0; j < N; j++)
            B.set(i, j, dist(rng));

    DistributedMatrix dA(A, numProcs);
    DistributedMatrix dB(B, numProcs);

    if (rank == 0)
        std::cout << "# np=" << numProcs
                  << " M=" << M << " N=" << N << " R=" << R
                  << " | t_compute t_comm t_total (seconds)" << std::endl;

    // multiplyTransposed imprime "t_compute t_comm" sur stdout depuis rank 0
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    dA.multiplyTransposed(dB);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_total = MPI_Wtime() - t_start;

    if (rank == 0)
        std::cout << t_total << std::endl;

    MPI_Finalize();
    return 0;
}
