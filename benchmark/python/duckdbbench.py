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


class Query11:
    def __init__(self, nation_key=0, fraction=0.0001):
        partsupp = TPCHPartSupp.load()
        supplier = TPCHSupplier.load()
        duckdb.sql("CREATE TABLE PartSupp AS SELECT * FROM partsupp")
        duckdb.sql("ALTER TABLE PartSupp ADD PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY);")
        duckdb.sql("CREATE TABLE Supplier AS SELECT * FROM supplier")
        duckdb.sql("ALTER TABLE Supplier ADD PRIMARY KEY (S_SUPPKEY);")
        self.query_str = f"""
            select
                PS_PARTKEY,
                sum(PS_SUPPLYCOST * PS_AVAILQTY) as value
            from
                PartSupp,
                Supplier
            where
                PS_SUPPKEY = s_suppkey
                and S_NATIONKEY = {nation_key}
            group by
                PS_PARTKEY having
                        sum(PS_SUPPLYCOST * PS_AVAILQTY) > (
                    select
                        sum(PS_SUPPLYCOST * PS_AVAILQTY) * {fraction}
                    from
                        PartSupp,
                        Supplier
                    where
                        ps_suppkey = s_suppkey
                        and s_nationkey = {nation_key}
                    )
        """


    def run(self):
        return duckdb.sql(self.query_str).show()


class Query9:
    def __init__(self, nation_key=0, fraction=0.0001):
        partsupp = TPCHPartSupp.load()
        supplier = TPCHSupplier.load()
        duckdb.sql("CREATE TABLE PartSupp AS SELECT * FROM partsupp")
        duckdb.sql("ALTER TABLE PartSupp ADD PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY);")
        duckdb.sql("CREATE TABLE Supplier AS SELECT * FROM supplier")
        duckdb.sql("ALTER TABLE Supplier ADD PRIMARY KEY (S_SUPPKEY);")
        self.query_str = f"""
            select
                PS_PARTKEY,
                sum(PS_SUPPLYCOST * PS_AVAILQTY) as value
            from
                PartSupp,
                Supplier
            where
                PS_SUPPKEY = s_suppkey
                and S_NATIONKEY = {nation_key}
            group by
                PS_PARTKEY having
                        sum(PS_SUPPLYCOST * PS_AVAILQTY) > (
                    select
                        sum(PS_SUPPLYCOST * PS_AVAILQTY) * {fraction}
                    from
                        PartSupp,
                        Supplier
                    where
                        ps_suppkey = s_suppkey
                        and s_nationkey = {nation_key}
                    )
        """


    def run(self):
        return duckdb.sql(self.query_str).show()


class Query1:
    def __init__(self, date=904694400):
        lineitem = TPCHLineItem.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        self.query_str = f"""
            select
                L_RETURNFLAG,
                sum(L_QUANTITY) as sum_qty,
                sum(L_EXTENDEDPRICE) as sum_base_price,
                sum(L_EXTENDEDPRICE * (1 - L_DISCOUNT)) as sum_disc_price,
                sum(L_EXTENDEDPRICE * (1 - L_DISCOUNT) * (1 + L_TAX)) as sum_charge,
                count(*) as count_order
            from
                LineItem
            where
                L_SHIPDATE <= {date}
            group by
                l_returnflag,
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


if __name__ == "__main__":
    q = Query1()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
