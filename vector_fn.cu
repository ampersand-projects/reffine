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

// same as above, but with sequential accesses for locality
__global__ void vector_fn_seq(int n, int* vec, int* res)
{
  // gridDim: # of blocks in a grid
  // blockDim: # of threads in a block

    int tid = threadIdx.x;
    int elements_per_block = (n + gridDim.x - 1) / gridDim.x;
    int block_start = blockIdx.x * elements_per_block;
    // int block_end = min(block_start + elements_per_block, n);
    int block_end = block_start + elements_per_block;
    
    int elements_per_thread = (block_end - block_start + blockDim.x - 1) / blockDim.x;
    int thread_start = block_start + tid * elements_per_thread;
    // int thread_end = min(thread_start + elements_per_thread, block_end);
    int thread_end = thread_start + elements_per_thread;
    
    // Sequential access with stride 1
    int temp_sum = 0;
    for (int i = thread_start; i < thread_end; i++) {
        temp_sum += vec[i];
    }
    atomicAdd(res, temp_sum);
}

int main() {
    int N = 1024;
    int* vec;
    int* res;
    cudaMallocManaged(&vec, N * sizeof(int));
    cudaMallocManaged(&res, sizeof(int));
    std::cout << "done alloc, about to initialize with N=" << N << std::endl;

    int true_res = 0;
    for (int i = 0; i < N; i++) {
      vec[i] = i;
      true_res += i;
    }
    int blockSize = 32;   // num threads per block
    int numBlocks = (N + blockSize - 1) / blockSize;
    
    *res = 0;
    std::cout << "done init, about to run vector_fn kernel, expected result = " << true_res << std::endl;
    vector_fn<<<numBlocks, blockSize>>>(N, vec, res);
    cudaDeviceSynchronize();

    *res = 0;
    std::cout << "done init, about to run vector_fn_seq kernel, expected result = " << true_res << std::endl;
    vector_fn_seq<<<numBlocks, blockSize>>>(N, vec, res);
    cudaDeviceSynchronize();

    std::cout << "vector_fn result: " << *res << std::endl;
    std::cout << "  -  Equal to expected: " << ((true_res==*res) ? "true" : "false") << std::endl;
    std::cout << "vector_fn_seq result: " << *res << std::endl;
    std::cout << "  -  Equal to expected: " << ((true_res==*res) ? "true" : "false") << std::endl;

    cudaFree(res);
    cudaFree(vec);

    return 0;
}
