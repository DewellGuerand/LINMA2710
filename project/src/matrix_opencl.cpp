#include "matrix_opencl.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

std::shared_ptr<KernelCache> MatrixCL::kernels_ = nullptr;

cl::Program loadAndBuildProgram(cl::Context context,
                                const std::vector<cl::Device>& devices,
                                const std::string& sourceCode,
                                const std::string& kernel_name_for_error)
{
    cl::Program program(context, sourceCode);
    try {
        program.build(devices);
    } catch (const cl::BuildError& err) {
        std::cerr << "OpenCL Build Error for kernel source '" << kernel_name_for_error << "':\n"
                  << err.what() << "(" << err.err() << ")" << std::endl;
        for (const auto& pair : err.getBuildLog()) {
            std::cerr << "Device " << pair.first.getInfo<CL_DEVICE_NAME>() << ":" << std::endl;
            std::cerr << pair.second << std::endl;
        }
        throw;
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error during program build for '" << kernel_name_for_error << "': "
                  << err.what() << " (" << err.err() << ")" << std::endl;
        throw;
    }
    return program;
}

// --- OpenCL Kernel Source Code ---

const std::string kernel_source_fill = R"(
    __kernel void fill(__global float* matrix, float value, int rows, int cols) {
        // TODO
        int i = get_global_id(0);
        if (i < rows*cols){
            matrix[i] = value ; 
        }
    }
)";

const std::string kernel_source_add = R"(
    __kernel void add(__global const float* A,
                      __global const float* B,
                      __global float* C,
                      int rows, int cols) {
        int i = get_global_id(0);
        if (i < rows*cols){
            C[i] = A[i] + B[i] ; 
        }
        // TODO
    }
)";

const std::string kernel_source_sub_mul = R"(
    __kernel void sub_mul(__global float* A,
                          __global const float* B,
                          float scalar,
                          int rows, int cols) {
        int i = get_global_id(0);
        if (i < rows*cols){
            A[i] = A[i] - scalar * B[i] ; 
        }
        // TODO
    }
)";

const std::string kernel_source_transpose = R"(
    __kernel void transpose(__global const float* A,
                            __global float* B,
                            int A_rows, int A_cols) {
        int i = get_global_id(0);  // ligne de A
        int j = get_global_id(1);  // colonne de A
        if (i < A_rows && j < A_cols) {
            B[j * A_rows + i] = A[i * A_cols + j];
        }
        // TODO
    }
)";
/*
const std::string kernel_source_matrix_mul = R"(
    __kernel void matrix_mul(__global const float* A,
                             __global const float* B,
                             __global float* C,
                             int A_rows, int A_cols, int B_cols) {
        int i = get_global_id(0);  // ligne de C
        int j = get_global_id(1);  // colonne de C
        if (i < A_rows && j < B_cols) {
            float sum = 0.0f;
            for (int k = 0; k < A_cols; k++) {
                sum += A[i * A_cols + k] * B[k * B_cols + j];
            }
            C[i * B_cols + j] = sum;
        }
        // TODO
    }
)"; */
const std::string kernel_source_matrix_mul = R"(
#define TILE_SIZE 16

__kernel void matrix_mul(
    __global const float* A,
    __global const float* B,
    __global float* C,
    int A_rows, int A_cols, int B_cols)
{
    // Tuiles en mémoire locale (partagée au sein du work-group)
    __local float tileA[TILE_SIZE][TILE_SIZE];
    __local float tileB[TILE_SIZE][TILE_SIZE];

    // Coordonnées globales du résultat C que ce work-item calcule
    int row = get_global_id(0);  // ligne de C
    int col = get_global_id(1);  // colonne de C

    // Coordonnées locales dans le work-group (pour charger les tuiles)
    int local_row = get_local_id(0);
    int local_col = get_local_id(1);

    float acc = 0.0f;

    // Nombre de tuiles à parcourir le long de la dimension partagée (A_cols)
    int num_tiles = (A_cols + TILE_SIZE - 1) / TILE_SIZE;

    for (int t = 0; t < num_tiles; t++) {

        // --- Chargement collaboratif de la tuile de A en mémoire locale ---
        int a_col = t * TILE_SIZE + local_col;
        if (row < A_rows && a_col < A_cols)
            tileA[local_row][local_col] = A[row * A_cols + a_col];
        else
            tileA[local_row][local_col] = 0.0f;

        // --- Chargement collaboratif de la tuile de B en mémoire locale ---
        int b_row = t * TILE_SIZE + local_row;
        if (b_row < A_cols && col < B_cols)
            tileB[local_row][local_col] = B[b_row * B_cols + col];
        else
            tileB[local_row][local_col] = 0.0f;

        // Attendre que tous les work-items du groupe aient fini de charger
        barrier(CLK_LOCAL_MEM_FENCE);

        // --- Calcul du produit partiel sur cette tuile ---
        for (int k = 0; k < TILE_SIZE; k++)
            acc += tileA[local_row][k] * tileB[k][local_col];

        // Attendre avant de charger la tuile suivante
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // Écriture du résultat final en mémoire globale
    if (row < A_rows && col < B_cols)
        C[row * B_cols + col] = acc;
}
)";

// --- KernelCache ---

void KernelCache::compileKernels(cl::Context context, const std::vector<cl::Device>& devices) {
    if (initialized) return;

    std::cerr << "Compiling OpenCL kernels..." << std::endl;
    try {
        cl::Program prog_fill = loadAndBuildProgram(context, devices, kernel_source_fill, "fill");
        kernel_fill = cl::Kernel(prog_fill, "fill");

        cl::Program prog_add = loadAndBuildProgram(context, devices, kernel_source_add, "add");
        kernel_add = cl::Kernel(prog_add, "add");

        cl::Program prog_sub_mul = loadAndBuildProgram(context, devices, kernel_source_sub_mul, "sub_mul");
        kernel_sub_mul = cl::Kernel(prog_sub_mul, "sub_mul");

        cl::Program prog_transpose = loadAndBuildProgram(context, devices, kernel_source_transpose, "transpose");
        kernel_transpose = cl::Kernel(prog_transpose, "transpose");

        cl::Program prog_matrix_mul = loadAndBuildProgram(context, devices, kernel_source_matrix_mul, "matrix_mul");
        kernel_matrix_mul = cl::Kernel(prog_matrix_mul, "matrix_mul");

        initialized = true;
        std::cerr << "OpenCL kernels compiled successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Failed to compile one or more OpenCL kernels. Aborting." << std::endl;
        throw;
    }
}

// --- MatrixCL Static Methods ---

void MatrixCL::initializeKernels(cl::Context context, const std::vector<cl::Device>& devices) {
    try {
        if (!kernels_ || !kernels_->initialized) {
            std::cerr << "Creating and compiling kernels..." << std::endl;
            kernels_ = std::make_shared<KernelCache>();
            kernels_->compileKernels(context, devices);
        }
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL error in kernel initialization: "
                  << err.what() << " (" << err.err() << ")" << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Exception in kernel initialization: " << e.what() << std::endl;
        throw;
    }
}

// --- MatrixCL Implementation ---

size_t MatrixCL::buffer_size_bytes() const {
    return static_cast<size_t>(rows_) * cols_ * sizeof(float);
}

MatrixCL::MatrixCL(int rows, int cols, cl::Context context, cl::CommandQueue queue, const std::vector<float>* initial_data)
    : rows_(rows), cols_(cols), context_(context), queue_(queue)
{
    buffer_ = cl::Buffer(context, CL_MEM_READ_WRITE, rows * cols * sizeof(float));

    if (initial_data != nullptr) {
        queue_.enqueueWriteBuffer(buffer_, CL_TRUE, 0, rows_ * cols_ * sizeof(float), initial_data->data());
        queue_.finish();  // attendre que l'écriture soit terminée
    }
    else {
        fill(0.0f);
    }
    // TODO

}

MatrixCL::MatrixCL(const MatrixCL& other)
    : rows_(other.rows_), cols_(other.cols_),
      context_(other.context_), queue_(other.queue_)
{
    std::vector<float> copied_data ; 
    copied_data = other.copyToHost() ; 
    buffer_ = cl::Buffer(context_, CL_MEM_READ_WRITE, rows_ * cols_ * sizeof(float));
    queue_.enqueueWriteBuffer(buffer_, CL_TRUE, 0, rows_ * cols_ * sizeof(float), copied_data.data());
    queue_.finish();  // attendre que l'écriture soit terminée
    
   


    // TODO
}

MatrixCL& MatrixCL::operator=(const MatrixCL& other)
{
    if (this == &other) return *this;
    MatrixCL temp(other);  // appelle le copy constructor
    rows_ = temp.rows_;
    cols_ = temp.cols_;
    context_ = temp.context_;
    queue_ = temp.queue_;
    buffer_ = temp.buffer_; 
    // TODO

    return *this;
}

int MatrixCL::numRows() const { return rows_; }
int MatrixCL::numCols() const { return cols_; }
cl::Context MatrixCL::getContext() const { return context_; }
cl::CommandQueue MatrixCL::getQueue() const { return queue_; }
const cl::Buffer& MatrixCL::getBuffer() const { return buffer_; }

std::vector<float> MatrixCL::copyToHost() const
{
    std::vector<float> host_data(static_cast<size_t>(rows_) * cols_);
    size_t size = buffer_size_bytes();
    if (size == 0) return host_data;
    queue_.finish();  // POur etre sur que la queue a finit ses opérations 
    queue_.enqueueReadBuffer(buffer_, CL_TRUE, 0, size, host_data.data());

    // TODO

    return host_data;
}

void MatrixCL::fill(float value)
{
    if (rows_ * cols_ == 0) return;
    kernels_->kernel_fill.setArg(0, buffer_);
    kernels_->kernel_fill.setArg(1, value);
    kernels_->kernel_fill.setArg(2, rows_);
    kernels_->kernel_fill.setArg(3,cols_);
    queue_.enqueueNDRangeKernel(kernels_->kernel_fill, cl::NullRange, cl::NDRange(rows_ * cols_ ), cl::NullRange);
    queue_.finish();


    // TODO
}

MatrixCL MatrixCL::operator+(const MatrixCL& other) const
{
    MatrixCL result(rows_, cols_, context_, queue_);
    if (rows_ * cols_ == 0) return result;
    kernels_->kernel_add.setArg(0,(*this).buffer_);
    kernels_->kernel_add.setArg(1,other.buffer_);
    kernels_->kernel_add.setArg(2,result.buffer_);
    kernels_->kernel_add.setArg(3, rows_);
    kernels_->kernel_add.setArg(4,cols_);
    result.queue_.enqueueNDRangeKernel(kernels_->kernel_add, cl::NullRange, cl::NDRange(rows_ * cols_ ), cl::NullRange);
    result.queue_.finish();
    




    // TODO

    return result;
}

MatrixCL MatrixCL::operator-(const MatrixCL& other) const
{
    MatrixCL result(*this);
    if (rows_ * cols_ == 0) return result;
    result.sub_mul(1.0f , other) ; 
     
    // TODO

    return result;
}

MatrixCL MatrixCL::operator*(float scalar) const
{
    MatrixCL result(rows_, cols_, context_, queue_);
    if (rows_ * cols_ == 0) return result;
    result.sub_mul(- scalar , *this) ; 
    
    // TODO

    return result;
}

MatrixCL MatrixCL::operator*(const MatrixCL& other) const
{
    int C_rows = this->rows_;
    int C_cols = other.cols_;
    MatrixCL result(C_rows, C_cols, context_, queue_);
    if (C_rows * C_cols == 0) return result;
    kernels_->kernel_matrix_mul.setArg(0, buffer_);
    kernels_->kernel_matrix_mul.setArg(1, other.buffer_);
    kernels_->kernel_matrix_mul.setArg(2, result.buffer_);
    kernels_->kernel_matrix_mul.setArg(3, rows_);
    kernels_->kernel_matrix_mul.setArg(4, cols_);
    kernels_->kernel_matrix_mul.setArg(5, other.cols_);
    queue_.enqueueNDRangeKernel(kernels_->kernel_matrix_mul, cl::NullRange,
                                cl::NDRange(rows_, other.cols_), cl::NullRange);
    queue_.finish();
    // TODO

    return result;
}

MatrixCL MatrixCL::transpose() const
{
    MatrixCL result(cols_, rows_, context_, queue_);
    if (rows_ * cols_ == 0) return result;
    kernels_->kernel_transpose.setArg(0, buffer_);
    kernels_->kernel_transpose.setArg(1, result.buffer_);
    kernels_->kernel_transpose.setArg(2, rows_);
    kernels_->kernel_transpose.setArg(3, cols_);
    queue_.enqueueNDRangeKernel(kernels_->kernel_transpose, cl::NullRange, 
                                 cl::NDRange(rows_, cols_), cl::NullRange);
    queue_.finish();
    
    return result;
    // TODO

    return result;
}

void MatrixCL::sub_mul(float scalar, const MatrixCL& other)
{
    if (rows_ * cols_ == 0) return;
    kernels_->kernel_sub_mul.setArg(0,buffer_);
    kernels_->kernel_sub_mul.setArg(1,other.buffer_);
    kernels_->kernel_sub_mul.setArg(2,scalar);
    kernels_->kernel_sub_mul.setArg(3, rows_);
    kernels_->kernel_sub_mul.setArg(4, cols_);
    queue_.enqueueNDRangeKernel(kernels_->kernel_sub_mul, cl::NullRange, cl::NDRange(rows_ * cols_ ), cl::NullRange);
    queue_.finish();


    // TODO
}
