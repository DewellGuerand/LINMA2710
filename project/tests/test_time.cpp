#include <iostream>
#include <chrono>
#include <random>
#include "matrix.hpp"

static void fill_random(Matrix &m, std::mt19937 &rng)
{
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (int i = 0; i < m.numRows(); ++i)
        for (int j = 0; j < m.numCols(); ++j)
            m.set(i, j, dist(rng));
}

/*
Arguments passé en entrée :
  - 1 = Opération :
        1 = addition                    A(MxN) + B(MxN)
        2 = soustraction                A(MxN) - B(MxN)
        3 = multiplication matricielle  A(MxN) * B(NxR)
        4 = multiplication scalaire     A(MxN) * 2.0
        5 = transposée                  A(MxN)
        6 = sub_mul                     A = A - 2.0 * B  (A,B : MxN)
  - 2 = M : lignes de A
  - 3 = N : colonnes de A (= lignes de B pour la multiplication matricielle)
  - 4 = R : colonnes de B (uniquement utile pour l'opération 3)

Affiche le temps d'exécution de l'opération seule (en secondes) sur stdout.
L'initialisation aléatoire est effectuée AVANT le chrono pour ne pas biaiser la mesure.
*/
int main(int argc, char const *argv[])
{
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <operation> <M> <N> <R>" << std::endl;
        return 1;
    }

    int operation = std::atoi(argv[1]);
    int M = std::atoi(argv[2]);
    int N = std::atoi(argv[3]);
    int R = std::atoi(argv[4]);

    std::mt19937 rng(42);
    double elapsed = 0.0;

    if (operation == 1) {
        Matrix A(M, N), B(M, N);
        fill_random(A, rng); fill_random(B, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        Matrix C = A + B;
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else if (operation == 2) {
        Matrix A(M, N), B(M, N);
        fill_random(A, rng); fill_random(B, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        Matrix C = A - B;
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else if (operation == 3) {
        Matrix A(M, N), B(N, R);
        fill_random(A, rng); fill_random(B, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        Matrix C = A * B;
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else if (operation == 4) {
        Matrix A(M, N);
        fill_random(A, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        Matrix C = A * 2.0;
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else if (operation == 5) {
        Matrix A(M, N);
        fill_random(A, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        Matrix C = A.transpose();
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else if (operation == 6) {
        Matrix A(M, N), B(M, N);
        fill_random(A, rng); fill_random(B, rng);
        auto t0 = std::chrono::high_resolution_clock::now();
        A.sub_mul(2.0, B);
        elapsed = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

    } else {
        std::cerr << "Opération inconnue : " << operation << std::endl;
        return 1;
    }

    std::cout << elapsed << std::endl;
    return 0;
}
