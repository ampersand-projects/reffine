from pandasbench import *
import duckdb

class Query6:
    def __init__(self):
        lineitem = TPCHLineItem.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        self.query_str = f"""
            select
                sum(L_EXTENDEDPRICE * L_DISCOUNT) as revenue
            from
                LineItem
            where
                L_SHIPDATE >= 820454400
                and L_SHIPDATE < 852076800
                and L_DISCOUNT between .05 - 0.01 and .05 + 0.01
                and L_QUANTITY < 24.5
            """

    def run(self):
        return duckdb.sql(self.query_str).fetchall()

class Query4:
    def __init__(self):
        lineitem = TPCHLineItem.load()
        orders = TPCHOrders.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        duckdb.sql("CREATE TABLE Orders AS SELECT * FROM orders")
        duckdb.sql("ALTER TABLE Orders ADD PRIMARY KEY (O_ORDERKEY);")
        self.query_str = f"""
            select
                O_ORDERPRIORITY,
                count(*) as ORDER_COUNT
            from
                Orders
            where
                O_ORDERDATE >= 700000000
                and O_ORDERDATE < 900000000
                and exists (
                    select
                        *
                    from
                        LineItem
                    where
                        L_ORDERKEY = O_ORDERKEY
                        and L_COMMITDATE < L_RECEIPTDATE
                )
            group by
                O_ORDERPRIORITY
        """

    def run(self):
        return duckdb.sql(self.query_str).fetchall()


if __name__ == "__main__":
    q = Query4()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
