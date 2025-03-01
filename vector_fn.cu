#include <iostream>

__global__ void vector_fn(int n, int* vec, int* res)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    
    int temp_sum = 0;
    for (int i = idx; i < n; i += stride) {
      temp_sum = temp_sum + vec[i];
    }
    atomicAdd(res, temp_sum);
}

int main() {
    int N = 1024;
    int* vec;
    int* res;
    cudaMallocManaged(&vec, N * sizeof(int));
    cudaMallocManaged(&res, sizeof(int));
    std::cout << "Done allocation, about to initialize with N=" << N << std::endl;

    *res = 0;
    int true_res = 0;
    for (int i = 0; i < N; i++) {
      vec[i] = i;
      true_res += i;
    }
    
    std::cout << "Done initialization, about to run kernels, expected result = " << true_res << std::endl;
    int blockSize = 32;
    int numBlocks = (N + blockSize - 1) / blockSize;
    vector_fn<<<numBlocks, blockSize>>>(N, vec, res);
    cudaDeviceSynchronize();

    std::cout << "vector_fn result: " << *res << std::endl;
    std::cout << "Equal to expected: " << ((true_res==*res) ? "true" : "false") << std::endl;

    cudaFree(res);
    cudaFree(vec);

    return 0;
}
