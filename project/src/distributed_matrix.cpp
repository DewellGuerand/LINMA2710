#include "distributed_matrix.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

// The matrix is split by columns across MPI processes.
// Each process stores a local Matrix with a subset of columns.
// Columns are distributed as evenly as possible.

DistributedMatrix::DistributedMatrix(const Matrix& matrix, int numProcs)
    : globalRows(matrix.numRows()),
      globalCols(matrix.numCols()),
      localCols(0),
      startCol(0),
      numProcesses(numProcs),
      rank(0),
      localData(matrix.numRows(), 1)
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // TODO
    // Transpose the matrix to have row major and directly have access to every data : 
    Matrix matrix_T = matrix.transpose() ; 

    // GEt process to debug 
    int name_length = MPI_MAX_PROCESSOR_NAME;
    char proc_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(proc_name, &name_length);

    std::printf("Process %d/%d is running on node <<%s>>\n", rank, numProcs, proc_name);

    // Vector to assign each column ect 
    std::vector<int> counts(numProcs);
    std::vector<int> displs(numProcs);

    const int base = globalCols / numProcs;
    const int rem = globalCols % numProcs;

    for (int p=0; p<numProcs; ++p) {
        counts[p] = base+(p<rem ? 1 : 0);
    }

    displs[0] = 0;
    for (int p=1; p<numProcs; ++p) {
        displs[p] = displs[p-1] + counts[p-1];
    }
    //Number of column 
    localCols = counts[rank];
    // STart columns
    startCol = displs[rank];

    // Number of element for each process
    std::vector<int> counts_elem(numProcs);
    std::vector<int> displs_elem(numProcs);
    for (int p = 0; p < numProcs; ++p) {
        counts_elem[p] = counts[p] * globalRows;
        displs_elem[p] = displs[p] * globalRows;
    }

    // BUffer d'envoie, uniquement le process 0 
    std::vector<double> send_buffer;

    if (rank == 0) {
        // matrix_T est (globalCols x globalRows)
        // On copie ligne par ligne dans le buffer
        send_buffer.resize(globalCols * globalRows);
        for (int i = 0; i < matrix_T.numRows(); ++i)
            for (int j = 0; j < matrix_T.numCols(); ++j)
                send_buffer[i * matrix_T.numCols() + j] = matrix_T.get(i, j);
    }

    std::vector<double> recv_buffer(localCols * globalRows);

    MPI_Scatterv(
        rank == 0 ? send_buffer.data() : nullptr,
        counts_elem.data(),
        displs_elem.data(),
        MPI_DOUBLE,
        recv_buffer.data(),
        localCols * globalRows,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );

    // Remplir localData depuis recv_buffer
    // recv_buffer est organisé comme (localCols x globalRows)
    // On retranspose pour avoir (globalRows x localCols)
    localData = Matrix(globalRows, localCols);
    for (int col = 0; col < localCols; ++col)
        for (int row = 0; row < globalRows; ++row)
            localData.set(row, col, recv_buffer[col * globalRows + row]);

    }

DistributedMatrix::DistributedMatrix(const DistributedMatrix& other)
    : globalRows(other.globalRows),
      globalCols(other.globalCols),
      localCols(other.localCols),
      startCol(other.startCol),
      numProcesses(other.numProcesses),
      rank(other.rank),
      localData(other.localData)
{
}

int DistributedMatrix::numRows() const { return globalRows; }
int DistributedMatrix::numCols() const { return globalCols; }
const Matrix& DistributedMatrix::getLocalData() const { return localData; }

double DistributedMatrix::get(int i, int j) const
{
    // TODO
    return 0.0;
}

void DistributedMatrix::set(int i, int j, double value)
{
    // TODO
}

int DistributedMatrix::globalColIndex(int localColIdx) const
{
    // TODO
    return -1;
}

int DistributedMatrix::localColIndex(int globalColIdx) const
{
    // TODO
    return -1;
}

int DistributedMatrix::ownerProcess(int globalColIdx) const
{
    // TODO
    return -1;
}

void DistributedMatrix::fill(double value)
{
    // TODO
}

DistributedMatrix DistributedMatrix::operator+(const DistributedMatrix& other) const
{
    // TODO
    return DistributedMatrix(*this);
}

DistributedMatrix DistributedMatrix::operator-(const DistributedMatrix& other) const
{
    // TODO
    return DistributedMatrix(*this);
}

DistributedMatrix DistributedMatrix::operator*(double scalar) const
{
    // TODO
    return DistributedMatrix(*this);
}

Matrix DistributedMatrix::transpose() const
{
    // TODO
    return Matrix(globalCols, globalRows);
}

void DistributedMatrix::sub_mul(double scalar, const DistributedMatrix& other)
{
    // TODO
}

DistributedMatrix DistributedMatrix::apply(const std::function<double(double)>& func) const
{
    // TODO
    return DistributedMatrix(*this);
}

DistributedMatrix DistributedMatrix::applyBinary(
    const DistributedMatrix& a,
    const DistributedMatrix& b,
    const std::function<double(double, double)>& func)
{
    // TODO
    return DistributedMatrix(a);
}

DistributedMatrix multiply(const Matrix& left, const DistributedMatrix& right)
{
    // TODO
    return DistributedMatrix(right);
}

Matrix DistributedMatrix::multiplyTransposed(const DistributedMatrix& other) const
{
    // TODO
    return Matrix(globalRows, other.globalRows);
}

double DistributedMatrix::sum() const
{
    // TODO
    return 0.0;
}

Matrix DistributedMatrix::gather() const
{
    // TODO

    // ON a besoin de convertir ça pour MPI
    std::vector<double> send_buffer(localCols * globalRows);
    for (int col = 0; col < localCols; ++col)
        for (int row = 0; row < globalRows; ++row)
            send_buffer[col * globalRows + row] = localData.get(row, col);

    std::vector<int> counts(numProcesses);
    std::vector<int> displs(numProcesses);

    const int base = globalCols / numProcesses;
    const int rem  = globalCols % numProcesses;

    for (int p = 0; p < numProcesses; ++p)
        counts[p] = (base + (p < rem ? 1 : 0)) * globalRows;

    displs[0] = 0;
    for (int p = 1; p < numProcesses; ++p)
        displs[p] = displs[p-1] + counts[p-1];

    std::vector<double> recv_buffer;
    if (rank == 0)
        recv_buffer.resize(globalCols * globalRows);

    MPI_Gatherv(
        send_buffer.data(),                      // chaque process envoie
        localCols * globalRows,                  // nb éléments envoyés
        MPI_DOUBLE,
        rank == 0 ? recv_buffer.data() : nullptr, // rank 0 reçoit
        counts.data(),
        displs.data(),
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );

    Matrix result(globalRows, globalCols);
    if (rank == 0) {
        for (int col = 0; col < globalCols; ++col)
            for (int row = 0; row < globalRows; ++row)
                result.set(row, col, recv_buffer[col * globalRows + row]);
    }

    return result;
}

void sync_matrix(Matrix *matrix, int rank, int src)
{
    // TODO
}
