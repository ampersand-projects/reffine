from pandasbench import *

import polars as pl

class Query6:
    def __init__(self):
        self.lineitem = pl.from_pandas(TPCHLineItem.load())

    def run(self):
        var1 = 820454400
        var2 = 852076800
        var3 = 0.04
        var4 = 0.06
        var5 = 24.5
    
        return (
            self.lineitem.filter(pl.col("L_SHIPDATE").is_between(var1, var2, closed="left"))
            .filter(pl.col("L_DISCOUNT").is_between(var3, var4))
            .filter(pl.col("L_QUANTITY") < var5)
            .with_columns(
                (pl.col("L_EXTENDEDPRICE") * pl.col("L_DISCOUNT")).alias("revenue")
            )
            .select(pl.sum("revenue"))
        )


class Query4:
    def __init__(self):
        self.lineitem = pl.from_pandas(TPCHLineItem.load())
        self.orders = pl.from_pandas(TPCHOrders.load())

    def run(self):
        var1 = 700000000
        var2 = 900000000

        return (
            # SQL exists translates to semi join in Polars API
            self.orders.join(
                (self.lineitem.filter(pl.col("L_COMMITDATE") < pl.col("L_RECEIPTDATE"))),
                left_on="O_ORDERKEY",
                right_on="L_ORDERKEY",
                how="semi",
            )
            .filter(pl.col("O_ORDERDATE").is_between(var1, var2, closed="left"))
            .group_by("O_ORDERPRIORITY")
            .agg(pl.len().alias("order_count"))
        )

class Query3:
    def __init__(self):
        self.customer = pl.from_pandas(TPCHCustomer.load())
        self.lineitem = pl.from_pandas(TPCHLineItem.load())
        self.orders = pl.from_pandas(TPCHOrders.load())

    def run(self):
        var1 = 1
        var2 = 795484800

        return (
            self.customer.filter(pl.col("C_MKTSEGMENT") == var1)
            .join(self.orders, left_on="C_CUSTKEY", right_on="O_CUSTKEY")
            .join(self.lineitem, left_on="O_ORDERKEY", right_on="L_ORDERKEY")
            .filter(pl.col("O_ORDERDATE") < var2)
            .filter(pl.col("L_SHIPDATE") > var2)
            .with_columns(
                (pl.col("L_EXTENDEDPRICE") * (1 - pl.col("L_DISCOUNT"))).alias("revenue")
            )
            .group_by("O_ORDERKEY")
            .agg(pl.sum("revenue"))
            .select(
                pl.col("O_ORDERKEY").alias("L_ORDERKEY"),
                "revenue",
            )
            .head(100)
        )


if __name__ == '__main__':
    q = Query3()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
