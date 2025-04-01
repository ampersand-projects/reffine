import cudf
import numpy as np
import pandas as pd
import time


# reference: 
#   - https://github.com/natalievolk/reffine/tree/vectorize
#   - https://developer.nvidia.com/rapids

def benchmark_aggregate(n=1025, iter=1000):
    
    
    values = pd.Series(range(n))
    values_gpu = cudf.Series(values)
    df_gpu = cudf.DataFrame({"values": values_gpu})

    total_time = 0
    for i in range(iter):
        start = time.time()
        
        result_gpu = df_gpu.sum()
        
        total_time += time.time() - start
        
    av_time = total_time / iter
    print(f"Average RAPIDS cuDF aggregation time: {av_time:.6f} seconds over {iter} iterations")
    print(result_gpu)
    
benchmark_aggregate()