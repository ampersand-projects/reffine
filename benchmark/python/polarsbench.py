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


class Query11:
    def __init__(self):
        self.partsupp = pl.from_pandas(TPCHPartSupp.load())
        self.supplier = pl.from_pandas(TPCHSupplier.load())

    def run(self, nation_key=0, fraction=0.0001):
        var1 = nation_key
        var2 = fraction
    
        q1 = (
            self.partsupp.join(self.supplier, left_on="PS_SUPPKEY", right_on="S_SUPPKEY")
            .filter(pl.col("S_NATIONKEY") == var1)
        )
        q2 = q1.select(
            (pl.col("PS_SUPPLYCOST") * pl.col("PS_AVAILQTY")).sum().round(2).alias("tmp")
            * var2
        )
    
        return (
            q1.group_by("PS_PARTKEY")
            .agg(
                (pl.col("PS_SUPPLYCOST") * pl.col("PS_AVAILQTY"))
                .sum()
                .alias("value")
            )
            .join(q2, how="cross")
            .filter(pl.col("value") > pl.col("tmp"))
            .select("PS_PARTKEY", "value")
        )


class Query9:
    def __init__(self):
        self.store_sales = pl.from_pandas(TPCDSStoreSales.load())

    def run(self):
        store_sales = self.store_sales

        def bucket(df: pl.DataFrame, low: int, high: int, threshold: int) -> float:
            f = df.filter(
                (pl.col("ss_quantity") >= low) & (pl.col("ss_quantity") <= high)
            )
            count = f.count()["ss_quantity"].item()
        
            if count > threshold:
                return f.select(pl.col("ss_ext_tax").sum()).item()
            else:
                return f.select(pl.col("ss_net_paid_inc_tax").sum()).item()
        
        
        # Compute buckets
        bucket1 = bucket(store_sales, 1, 20, 1071)
        bucket2 = bucket(store_sales, 21, 40, 39161)
        bucket3 = bucket(store_sales, 41, 60, 29434)
        bucket4 = bucket(store_sales, 61, 80, 6568)
        bucket5 = bucket(store_sales, 81, 100, 21216)
        
        # Final result as Polars DataFrame
        result = pl.DataFrame({
            "bucket1": [bucket1],
            "bucket2": [bucket2],
            "bucket3": [bucket3],
            "bucket4": [bucket4],
            "bucket5": [bucket5],
        })
        
        return result

class Query1:
    def __init__(self):
        self.lineitem = pl.from_pandas(TPCHLineItem.load())

    def run(self):
        var1 = 904694400

        return (
            self.lineitem.filter(pl.col("L_SHIPDATE") <= var1)
            .group_by("L_RETURNFLAG")
            .agg(
                pl.sum("L_QUANTITY").alias("sum_qty"),
                pl.sum("L_EXTENDEDPRICE").alias("sum_base_price"),
                (pl.col("L_EXTENDEDPRICE") * (1.0 - pl.col("L_DISCOUNT")))
                .sum()
                .alias("sum_disc_price"),
                (
                    pl.col("L_EXTENDEDPRICE")
                    * (1.0 - pl.col("L_DISCOUNT"))
                    * (1.0 + pl.col("L_TAX"))
                )
                .sum()
                .alias("sum_charge"),
                pl.len().alias("count_order"),
            )
        )

class PLRSelectBench:
    def __init__(self):
        self.df = pl.from_pandas(FakeData().load())

    def run(self):
        return (
            self.df.filter(pl.col("t") % 2 == 0).select(pl.col("t"))
        )

if __name__ == '__main__':
    q = PLRSelectBench()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
