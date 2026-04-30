#include "matrix.hpp"
#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <iostream>

Matrix::Matrix(int rows, int cols)
    : rows(0), cols(0)
{
    // TODO
    this->rows = rows;
    this->cols = cols; 
    this->data = std::vector<double>(rows * cols, 0.0); // On initiaise le vecteur 
}

Matrix::Matrix(const Matrix &other)
    : rows(0), cols(0)
{
    // TODO
    this->rows = other.rows;
    this->cols = other.cols;
    this->data = other.data;
}

int Matrix::numRows() const
{
    return this->rows;

}

int Matrix::numCols() const
{
    return this->cols; 
}

double Matrix::get(int i, int j) const
{
    try
    {
        return this->data[this->cols * i + j]; // i ème ligne + j colonne comme ça on a la mémoire conjointe

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void Matrix::set(int i, int j, double value)
{
    // TODO
    (*this).data[i * (*this).cols + j  ] = value ; 
}

void Matrix::fill(double value)
{
    // TODO
    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < this->cols ; j++){
           this->data[i * this->cols + j] = value ; 
        }
    }
}

Matrix Matrix::operator+(const Matrix &other) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows;
    result.cols = this->cols;
    result.data = std::vector<double>(this->rows * this->cols, 0.0);

    #pragma omp parallel for
    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < this->cols ; j++){
            result.data[i * result.cols + j] = this->data[i * this->cols + j] + other.get(i, j);
        }
    }
    
    
    
    return result;
    
}

Matrix Matrix::operator-(const Matrix &other) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows;
    result.cols = this->cols;
    result.data = std::vector<double>(this->rows * this->cols, 0.0);
    #pragma omp parallel for
    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < this->cols ; j++){
            result.data[i * result.cols + j] = this->data[i * this->cols + j] - other.get(i, j);
        }
    }
    
    return result;
}
/*
Matrix Matrix::operator*(const Matrix &other) const
{
    
    
    int R = this->rows, K = this->cols, C = other.numCols();
    Matrix result(R, C);
    Matrix otherT = other.transpose();         
    #pragma omp parallel for
    for (int i = 0; i < R; i++){
        for (int j = 0; j < C; j++){
            double sum = 0.0;
            for (int k = 0; k < K; k++){
                sum += this->data[i * K + k] * otherT.data[j * K + k];
            }
            result.data[i * C + j] = sum;
        }
    }
    return result;
    
    
    
}
*/

Matrix Matrix::operator*(const Matrix &other) const
{
    const int R = this->rows;
    const int K = this->cols;
    const int C = other.numCols();

    Matrix result(R, C);
    Matrix otherT = other.transpose();

    const double* __restrict__ A   = this->data.data();
    const double* __restrict__ BT  = otherT.data.data();
    double*       __restrict__ Res = result.data.data();

    constexpr int TILE_I = 32;
    constexpr int TILE_J = 32;
    constexpr int TILE_K = 64;

    #pragma omp parallel for schedule(static) num_threads(16) collapse(2)
    for (int ii = 0; ii < R; ii += TILE_I)
    for (int jj = 0; jj < C; jj += TILE_J)
    {
        const int iEnd = std::min(ii + TILE_I, R);
        const int jEnd = std::min(jj + TILE_J, C);

        for (int kk = 0; kk < K; kk += TILE_K)
        {
            const int kEnd = std::min(kk + TILE_K, K);

            for (int i = ii; i < iEnd; i++)
            {
                const double* rowA = A + i * K;
                for (int j = jj; j < jEnd; j++)
                {
                    const double* rowBT = BT + j * K;
                    double sum = 0.0;

                    #pragma omp simd reduction(+:sum)
                    for (int k = kk; k < kEnd; k++)
                        sum += rowA[k] * rowBT[k];

                    Res[i * C + j] += sum;
                }
            }
        }
    }

    return result;
}
/**/
Matrix Matrix::operator*(double scalar) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows;
    result.cols = this->cols;
    result.data = std::vector<double>(this->rows * this->cols, 0.0);
    #pragma omp parallel for
    for (int i = 0 ; i < result.rows ; i++){
        for (int j = 0 ; j < result.cols ; j++){
            result.data[i * result.cols + j] = this->data[i * this->cols + j] * scalar;
        }
    }
    return result;
}

Matrix Matrix::transpose() const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->cols;
    result.cols = this->rows;
    result.data = std::vector<double>(result.rows * result.cols, 0.0);
    #pragma omp parallel for
    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < this->cols ; j++){
            result.data[j * result.cols + i] = this->data[i * this->cols + j];
        }
    }
    return result;
}

Matrix Matrix::apply(const std::function<double(double)> &func) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows ; 
    result.cols = this->cols ;
    result.data = std::vector<double>(result.rows * result.cols, 0.0);
    #pragma omp parallel for
    for (int i = 0 ; i < result.rows ; i++){
        for (int j = 0 ; j < result.cols ; j++){
            result.data[i * result.cols + j] = func(this->data[i * this->cols + j]);
        }
    }
    return result;
}

void Matrix::sub_mul(double scalar, const Matrix &other)
{
    *this = *this - other * scalar;
    // TODO
}
