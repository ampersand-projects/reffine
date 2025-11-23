from pandasbench import *

import polars as pl

def query6(lineitem):
    var1 = 820454400
    var2 = 852076800
    var3 = 0.04
    var4 = 0.06
    var5 = 24.5

    return (
        lineitem.filter(pl.col("L_SHIPDATE").is_between(var1, var2, closed="left"))
        .filter(pl.col("L_DISCOUNT").is_between(var3, var4))
        .filter(pl.col("L_QUANTITY") < var5)
        .with_columns(
            (pl.col("L_EXTENDEDPRICE") * pl.col("L_DISCOUNT")).alias("revenue")
        )
        .select(pl.sum("revenue"))
    )

if __name__ == '__main__':
    lineitem = pl.from_pandas(TPCHLineItem.load())
    import time
    start = time.time()
    out = query6(lineitem)
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
