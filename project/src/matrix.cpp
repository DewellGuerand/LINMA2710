#include "matrix.hpp"
#include <stdexcept>
#ifdef _OPENMP
#include <omp.h>
#endif

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
    return this->data[this->cols * i + j]; // i ème ligne + j colonne comme ça on a la mémoire conjointe
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

    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < this->cols ; j++){
            result.data[i * result.cols + j] = this->data[i * this->cols + j] - other.get(i, j);
        }
    }
    
    return result;
}

Matrix Matrix::operator*(const Matrix &other) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows;
    result.cols = other.numCols();
    result.data = std::vector<double>(result.rows * result.cols, 0.0);

    for (int i = 0 ; i < this->rows ; i++){
        for (int j = 0 ; j < result.cols ; j++){
            for (int k = 0 ; k < this->cols ; k++){
                result.data[i * result.cols + j] += this->data[i * this->cols + k] * other.get(k, j);
            }
        }
    }
    return result;
}

Matrix Matrix::operator*(double scalar) const
{
    Matrix result(0, 0);
    // TODO
    result.rows = this->rows;
    result.cols = this->cols;
    result.data = std::vector<double>(this->rows * this->cols, 0.0);

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
