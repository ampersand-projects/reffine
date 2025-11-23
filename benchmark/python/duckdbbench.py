from pandasbench import *
import duckdb

class Query6:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()
        lineitem = self.lineitem
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        self.query_str = f"""
            select
                sum(L_EXTENDEDPRICE * L_DISCOUNT) as revenue
            from
                lineitem
            where
                L_SHIPDATE >= 820454400
                and L_SHIPDATE < 852076800
                and L_DISCOUNT between .05 - 0.01 and .05 + 0.01
                and L_QUANTITY < 24.5
            """

    def run(self):
        return duckdb.sql(self.query_str)

class Query4:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()
        self.orders = TPCHOrders.load()
        self.query_str = f"""
            select
                O_ORDERPRIORITY,
                count(*) as ORDER_COUNT
            from
                orders
            where
                O_ORDERDATE >= 700000000
                and O_ORDERDATE < 900000000
                and exists (
                    select
                        *
                    from
                        lineitem
                    where
                        L_ORDERKEY = O_ORDERKEY
                        and L_COMMITDATE < L_RECEIPTDATE
                )
            group by
                O_ORDERPRIORITY
        """

    def run(self):
        lineitem = self.lineitem
        orders = self.orders
        return duckdb.sql(self.query_str)

if __name__ == "__main__":
    q = Query4()
    import time
    start = time.time()
    out = q.run()
    print(out)
    end = time.time()
    print("Time: ", (end - start) * 1000)
