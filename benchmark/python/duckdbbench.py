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


class Query3:
    def __init__(self, segment = 1, date = 795484800):
        lineitem = TPCHLineItem.load()
        orders = TPCHOrders.load()
        customer = TPCHCustomer.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        duckdb.sql("CREATE TABLE Orders AS SELECT * FROM orders")
        duckdb.sql("ALTER TABLE Orders ADD PRIMARY KEY (O_ORDERKEY);")
        duckdb.sql("CREATE TABLE Customer AS SELECT * FROM customer")
        duckdb.sql("ALTER TABLE Customer ADD PRIMARY KEY (C_CUSTKEY);")
        self.query_str = f"""
            select
                L_ORDERKEY,
                sum(L_EXTENDEDPRICE * (1 - L_DISCOUNT)) as revenue,
            from
                Customer,
                Orders,
                LineItem
            where
                C_MKTSEGMENT = {segment}
                and C_CUSTKEY = O_CUSTKEY
                and L_ORDERKEY = O_ORDERKEY
                and O_ORDERDATE < {date}
                and L_SHIPDATE > {date}
            group by
                L_ORDERKEY
            limit 100
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


if __name__ == "__main__":
    q = Query3()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
