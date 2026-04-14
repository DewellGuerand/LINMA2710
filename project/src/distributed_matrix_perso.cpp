#include "distributed_matrix.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>


// A commenter juste pour les tests
#include <iostream>


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
    
    if(j - startCol >= 0 ){
        if (j-startCol< localCols){
            return localData.get(i,j-startCol) ; 
        }
    }

    else {
        throw std::runtime_error("Column " + std::to_string(j) + " not owned by this process");
        return -1 ; 
    };
    return -1; 
}

void DistributedMatrix::set(int i, int j, double value)
{
    // TODO
    if(j - startCol>= 0 ){
        if (j-startCol< localCols){
            localData.set(i,j-startCol, value) ; 
        }
    }
    else throw std::runtime_error("Column " + std::to_string(j) + " not owned by this process");


    
}

/*
double DistributedMatrix::get(int i, int j) const
{
    int localCol = j - startCol;
    if (localCol < 0 || localCol >= localCols)
        throw std::runtime_error("Column " + std::to_string(j) + " not owned by this process");
    return localData.get(i, localCol);
}

void DistributedMatrix::set(int i, int j, double value)
{
    int localCol = j - startCol;
    if (localCol < 0 || localCol >= localCols)
        throw std::runtime_error("Column " + std::to_string(j) + " not owned by this process");
    localData.set(i, localCol, value);
}


*/

int DistributedMatrix::globalColIndex(int localColIdx) const
{
    // TODO
    // Number of total column :
    
    return startCol + localColIdx ;
}

int DistributedMatrix::localColIndex(int globalColIdx) const
{
    // TODO

    return globalColIdx - startCol;
}

int DistributedMatrix::ownerProcess(int globalColIdx) const
{
    // TODO
    if (startCol <= globalColIdx){
        if (globalColIdx <= startCol + localCols){
            return rank ;
        }
    }
    return 0;
    
}

void DistributedMatrix::fill(double value)
{
    // TODO
    Matrix result(globalRows , localCols) ;
    result.fill(value);
    localData = result ; 
    
}

DistributedMatrix DistributedMatrix::operator+(const DistributedMatrix& other) const
{
    // TODO
    DistributedMatrix result(other);
    result.localData = localData + other.localData ; 
    return result;
}

DistributedMatrix DistributedMatrix::operator-(const DistributedMatrix& other) const
{
    // TODO
    DistributedMatrix result(other);
    result.localData = localData - other.localData ; 
    return result;
}

DistributedMatrix DistributedMatrix::operator*(double scalar) const
{
    // TODO
    DistributedMatrix result(*this);
    result.localData = localData * scalar ; 
    return result;
}

Matrix DistributedMatrix::transpose() const
{
    // TODO
    Matrix result(globalRows , globalCols);
    result = (*this).gather() ; 
    

    return result.transpose();
}

void DistributedMatrix::sub_mul(double scalar, const DistributedMatrix& other)
{
    // TODO
    DistributedMatrix result(other);
    localData = localData - other.localData * scalar ; 
    

}

DistributedMatrix DistributedMatrix::apply(const std::function<double(double)>& func) const
{
    // TODO
    DistributedMatrix result(*this);           
    result.localData = localData.apply(func);  
    return result;
}

DistributedMatrix DistributedMatrix::applyBinary(
    const DistributedMatrix& a,
    const DistributedMatrix& b,
    const std::function<double(double, double)>& func)
{
    // TODO
    DistributedMatrix result(a);
    Matrix newData(a.globalRows, a.localCols);
    for (int i = 0; i < a.globalRows; i++)
        for (int j = 0; j < a.localCols; j++)
            newData.set(i, j, func(a.localData.get(i, j), b.localData.get(i, j)));
    result.localData = newData;
    return result;
}

DistributedMatrix multiply(const Matrix& left, const DistributedMatrix& right)
{
    // TODO
    DistributedMatrix result(right) ;
    // Row doivent être update parce que sinon mismatch de dimention 
    result.globalRows = left.numRows();
    result.localData = left * right.localData ;
    return result;
}


// A commenter juste pour les tests

Matrix DistributedMatrix::multiplyTransposed(const DistributedMatrix& other) const
{
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();
    Matrix localContrib = localData * other.localData.transpose();
    double t_compute = MPI_Wtime() - t0;

    int resultRows = globalRows;
    int resultCols = other.globalRows;
    int totalElems = resultRows * resultCols;

    std::vector<double> sendBuf(totalElems);
    for (int i = 0; i < resultRows; ++i)
        for (int j = 0; j < resultCols; ++j)
            sendBuf[i * resultCols + j] = localContrib.get(i, j);

    std::vector<double> recvBuf(totalElems);

    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();
    MPI_Allreduce(sendBuf.data(), recvBuf.data(), totalElems, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    double t_comm = MPI_Wtime() - t1;

    double max_compute, max_comm;
    MPI_Reduce(&t_compute, &max_compute, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_comm,    &max_comm,    1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0)
        std::cout << max_compute << " " << max_comm << std::endl;

    Matrix result(resultRows, resultCols);
    for (int i = 0; i < resultRows; ++i)
        for (int j = 0; j < resultCols; ++j)
            result.set(i, j, recvBuf[i * resultCols + j]);

    return result;
}

// For computation time 
/*


Matrix DistributedMatrix::multiplyTransposed(const DistributedMatrix& other) const
{
    

    Matrix localContrib = localData * other.localData.transpose();

    int resultRows = globalRows;
    int resultCols = other.globalRows;
    int totalElems = resultRows * resultCols;

    std::vector<double> sendBuf(totalElems);
    for (int i = 0; i < resultRows; ++i)
        for (int j = 0; j < resultCols; ++j)
            sendBuf[i * resultCols + j] = localContrib.get(i, j);

    std::vector<double> recvBuf(totalElems);
    MPI_Allreduce(sendBuf.data(), recvBuf.data(), totalElems, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD); // Every process get the reduced value, because it is testec for every of them  

    Matrix result(resultRows, resultCols);
    for (int i = 0; i < resultRows; ++i)
        for (int j = 0; j < resultCols; ++j)
            result.set(i, j, recvBuf[i * resultCols + j]);

    return result;
}
*/





double DistributedMatrix::sum() const
{
    // TODO
    double local_sum_send = 0 ; 
    // EAch process has a GLobalROws x localCOls so we will need to sum everything and then send it by a AllREduce in order to have the correct number 
    for (int i = 0; i < globalRows; i++)
    {
        
        for (int j = 0; j < localCols; j++)
        {
            local_sum_send += localData.get(i,j) ; 
        }
            
        
        
    }

    double global_sum =0; 
    MPI_Allreduce(&local_sum_send , &global_sum , 1, MPI_DOUBLE , MPI_SUM , MPI_COMM_WORLD);

    
    
    
    return global_sum;
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

    std::vector<double> recv_buffer(globalRows * globalCols);

    MPI_Allgatherv(
        send_buffer.data(),                      // chaque process envoie
        localCols * globalRows,                  // nb éléments envoyés
        MPI_DOUBLE,
        recv_buffer.data(), 
        counts.data(),
        displs.data(),
        MPI_DOUBLE,
        MPI_COMM_WORLD
    );

    Matrix result(globalRows, globalCols);
   
    for (int col = 0; col < globalCols; ++col)
        for (int row = 0; row < globalRows; ++row)
                result.set(row, col, recv_buffer[col * globalRows + row]);
            

    return result;
}

void sync_matrix(Matrix *matrix, int rank, int src)
{
    // Broadcaster les dimensions
    int dims[2] = { matrix->numRows(), matrix->numCols() };                                                 
    MPI_Bcast(dims, 2, MPI_INT, src, MPI_COMM_WORLD);
                                                                                                            
    int rows = dims[0], cols = dims[1];
                                                                                                            
    // Les processus non-src créent une matrice de la bonne taille                                          
    if (rank != src) {
        *matrix = Matrix(rows, cols);                                                                       
    }           
                                                                                                            
    // Copier les données dans un buffer contigu                                                            
    std::vector<double> buf(rows * cols);
                                                                                                            
    if (rank == src) {
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)                                                                  
                buf[i * cols + j] = matrix->get(i, j);
    }                                                                                                       
                
    MPI_Bcast(buf.data(), rows * cols, MPI_DOUBLE, src, MPI_COMM_WORLD);                                    

    if (rank != src) {                                                                                      
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)                                                                  
                matrix->set(i, j, buf[i * cols + j]);
    }                                                                                                       
}  
