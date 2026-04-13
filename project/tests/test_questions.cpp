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


int main(int argc, char const *argv[])
{
    


    std::mt19937 rng(42);
    int M = 2 ; 
    int N = 2 ; 
    Matrix A(M ,N) ; 
    fill_random(A , rng) ; 
    Matrix B(M,N) ; 
    B = Matrix(A) ;
    int i = 1 ;
    int j = 1 ;  
    A.set(i , j , 3) ; 
    std::cout << "Element A[" << i << j <<"] = " <<A.get(i,j) << std::endl;
    std::cout << "Element B[" << i << j <<"] = " <<B.get(i,j) << std::endl;

    return 0;
}
