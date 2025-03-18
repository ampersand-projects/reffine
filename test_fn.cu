__global__ void vector_fn(int* res)
{
    atomicAdd(res, 5);
}