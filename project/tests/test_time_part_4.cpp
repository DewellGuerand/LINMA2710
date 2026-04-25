#include "matrix_opencl.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <chrono>
#include <random>

cl::Context context;
cl::CommandQueue queue;

void setupOpenCL() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    assert(!platforms.empty());

    cl::Platform platform = platforms.front();
    std::cerr << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if (devices.empty())
        platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
    assert(!devices.empty());

    cl::Device device = devices.front();
    std::cerr << "Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    context = cl::Context(device);
    // CL_QUEUE_PROFILING_ENABLE allows event-based profiling if needed later
    queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE);

    MatrixCL::initializeKernels(context, {device});
}

MatrixCL fill_random(int m, int n, std::mt19937& rng, std::uniform_real_distribution<float>& dist) {
    std::vector<float> data(m * n);
    for (int i = 0; i < m * n; i++)
        data[i] = dist(rng);
    return MatrixCL(m, n, context, queue, &data);
}

int main(int argc, char** argv) {
    try {
        setupOpenCL();

        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " <M> <N> <R>" << std::endl;
            return 1;
        }

        int M = std::atoi(argv[1]);
        int N = std::atoi(argv[2]);
        int R = std::atoi(argv[3]);

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        MatrixCL matA = fill_random(M, N, rng, dist);  // [M x N]
        MatrixCL matB = fill_random(N, R, rng, dist);  // [N x R]

        // Warm-up run (avoids counting first-time GPU JIT overhead)
        { MatrixCL tmp = matA * matB; }

        // Timed runs
        const int RUNS = 5;
        double total_ms = 0.0;

        for (int r = 0; r < RUNS; r++) {
            auto t0 = std::chrono::high_resolution_clock::now();

            MatrixCL result = matA * matB;
            // queue_.finish() is called inside operator*, so GPU work is complete here

            auto t1 = std::chrono::high_resolution_clock::now();
            total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        double avg_ms = total_ms / RUNS;

        // Output CSV row: M,N,R,time_ms
        std::cout << M << "," << N << "," << R << "," << avg_ms << std::endl;

    } catch (const cl::BuildError& err) {
        std::cerr << "OpenCL Build Error: " << err.what() << " (" << err.err() << ")" << std::endl;
        for (const auto& pair : err.getBuildLog())
            std::cerr << "Build Log (" << pair.first.getInfo<CL_DEVICE_NAME>() << "):\n" << pair.second << std::endl;
        return 1;
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error: " << err.what() << " (" << err.err() << ")" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
