CXX = g++
MPICXX = mpic++
CXXFLAGS = -std=c++17 -Wall -Wextra 
O1FLAGS = -O1
O2FLAGS = -O2
O3FLAGS = -O3
FASTMATHFLAGS = -ffast-math
NATIVEFLAGS = -march=native



INCLUDE = -Iinclude

# OpenMP flag: set to empty to disable (e.g., on macOS without OpenMP)
OPENMP_FLAGS ?= -fopenmp

SRC_DIR ?= src

# --- Part 1 & 2: Basic + OpenMP Matrix ---
test_matrix: tests/test_matrix.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O3FLAGS) $(NATIVEFLAGS) $(FASTMATHFLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix tests/test_matrix.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_O1: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O1FLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_O1 tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_O2: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O2FLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_O2 tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_O3: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O3FLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_O3 tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_fastmath: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(FASTMATHFLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_fastmath tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_native: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(NATIVEFLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_native tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_fO3: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O3FLAGS) $(FASTMATHFLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_fO3 tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_matrix_perso_fO3native: tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(O3FLAGS) $(NATIVEFLAGS) $(FASTMATHFLAGS) $(OPENMP_FLAGS) $(INCLUDE) -o test_matrix_perso_fO3native tests/test_time_part_1_2.cpp $(SRC_DIR)/matrix.cpp

test_questions: tests/test_questions.cpp $(SRC_DIR)/matrix.cpp include/matrix.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o test_questions tests/test_questions.cpp $(SRC_DIR)/matrix.cpp


run_matrix: test_matrix
	./test_matrix

# --- Part 3: Distributed Matrix (MPI) ---
test_distributed: tests/test_distributed.cpp $(SRC_DIR)/distributed_matrix.cpp $(SRC_DIR)/matrix.cpp include/distributed_matrix.hpp include/matrix.hpp
	$(MPICXX) $(CXXFLAGS) $(INCLUDE) -o test_distributed tests/test_distributed.cpp $(SRC_DIR)/distributed_matrix.cpp $(SRC_DIR)/matrix.cpp

test_distributed_perso: tests/test_time_part_3.cpp $(SRC_DIR)/distributed_matrix_perso.cpp $(SRC_DIR)/matrix.cpp include/distributed_matrix.hpp include/matrix.hpp
	$(MPICXX) $(CXXFLAGS) $(INCLUDE) -o test_distributed_perso tests/test_time_part_3.cpp $(SRC_DIR)/distributed_matrix_perso.cpp $(SRC_DIR)/matrix.cpp

run_distributed_perso: test_distributed_perso
	mpirun -np 4 ./test_distributed_perso 100 100 100


run_distributed: test_distributed
	mpirun -np 4 ./test_distributed

# --- Part 3 (alternative): Data-parallel neural network ---
nn_data_parallel: src/nn_data_parallel.cpp src/matrix.cpp include/matrix.hpp
	$(MPICXX) $(CXXFLAGS) $(INCLUDE) -o nn_data_parallel src/nn_data_parallel.cpp src/matrix.cpp

run_nn: nn_data_parallel
	mpirun -np 1 ./nn_data_parallel

# --- Part 4: OpenCL Matrix ---
test_opencl: tests/test_opencl.cpp $(SRC_DIR)/matrix_opencl.cpp include/matrix_opencl.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o test_opencl tests/test_opencl.cpp $(SRC_DIR)/matrix_opencl.cpp -lOpenCL

run_opencl: test_opencl
	./test_opencl

test_time_opencl_perso: tests/test_time_part_4.cpp $(SRC_DIR)/matrix_opencl.cpp include/matrix_opencl.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o test_opencl_perso tests/test_time_part_4.cpp $(SRC_DIR)/matrix_opencl.cpp -lOpenCL

run_time_opencl_perso: test_time_part_4
	./test_time_part_4 512 512 512



# --- Utilities ---
all: test_matrix test_distributed test_opencl

clean:
	rm -f test_matrix test_distributed test_opencl test_matrix_perso_O1 test_matrix_perso_O2 test_matrix_perso_O3 test_matrix_perso_fastmath test_matrix_perso_native test_matrix_perso_fO3 test_matrix_perso_fO3native test_distributed_perso test_questions nn_data_parallel

.PHONY: all clean run_matrix run_distributed run_opencl
