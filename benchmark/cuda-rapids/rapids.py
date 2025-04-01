import cudf
import numpy as np
import pandas as pd
import time


# reference: 
#   - https://github.com/natalievolk/reffine/tree/vectorize
#   - https://developer.nvidia.com/rapids

def benchmark_aggregate(n=1025, iter=10000):
    values = pd.Series(range(n))
    values_gpu = cudf.Series(values)
    df_gpu = cudf.DataFrame({"values": values_gpu})

    total_time = 0
    for i in range(iter):
        start = time.time()
        
        res_gpu = df_gpu.sum()
        
        total_time += time.time() - start
        
    av_time = total_time / iter
    print(f"Average RAPIDS cuDF aggregation time: {av_time:.6f} seconds over {iter} iterations")
    print(res_gpu)
    
    
def benchmark_transform(n=1025, iter=10000):
    values = pd.Series(range(n))
    values_gpu = cudf.Series(values)
    df_gpu = cudf.DataFrame({"values": values_gpu})

    total_time = 0
    for i in range(iter):
        start = time.time()
        
        res_gpu = df_gpu + 1
        
        total_time += time.time() - start
        
    av_time = total_time / iter
    print(f"Average RAPIDS cuDF transform time: {av_time:.6f} seconds over {iter} iterations")
    print(res_gpu)
    
def benchmark_select(n=1025, iter=10000):
    values = pd.Series(range(n))
    values_gpu = cudf.Series(values)
    df_gpu = cudf.DataFrame({"values": values_gpu})

    total_time = 0
    for i in range(iter):
        start = time.time()
        
        res_gpu = df_gpu.map(lambda x: x if x % 2 == 0 else 0)
        
        total_time += time.time() - start
        
    av_time = total_time / iter
    print(f"Average RAPIDS cuDF select time: {av_time:.6f} seconds over {iter} iterations")
    print(res_gpu)
    
benchmark_aggregate()
benchmark_transform()
benchmark_select()