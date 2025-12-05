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
        return duckdb.sql(self.query_str).show()

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
        return duckdb.sql(self.query_str).show()


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
        self.query_str = f"""
            with ps_values as (
                    select
                        ps_partkey,
                        sum(ps_supplycost * ps_availqty) as value
                    from
                        partsupp,
                        supplier
                    where
                        ps_suppkey = s_suppkey
                        and s_nationkey = {nation_key}
                    group by
                        ps_partkey
            )
            select
                ps.ps_partkey,
                ANY_VALUE(psv.value),
            from
                partsupp ps,
                ps_values psv
            where
                ps.ps_partkey = psv.ps_partkey
                and psv.value > (
                    select sum(value) * {fraction} from ps_values
                )
            group by
                ps.ps_partkey
            order by
                ps.ps_partkey
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


class Query2:
    def __init__(self, nation_key=0, size=15):
        part = TPCHPart.load()
        supplier = TPCHSupplier.load()
        partsupp = TPCHPartSupp.load()
        duckdb.sql("CREATE TABLE Part AS SELECT * FROM part")
        duckdb.sql("ALTER TABLE Part ADD PRIMARY KEY (P_PARTKEY);")
        duckdb.sql("CREATE TABLE Supplier AS SELECT * FROM supplier")
        duckdb.sql("ALTER TABLE Supplier ADD PRIMARY KEY (S_SUPPKEY);")
        duckdb.sql("CREATE TABLE PartSupp AS SELECT * FROM partsupp")
        duckdb.sql("ALTER TABLE PartSupp ADD PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY);")
        self.query_str = f"""
            select
                P_PARTKEY,
                S_SUPPKEY,
                S_ACCTBAL,
            from
                Part,
                Supplier,
                PartSupp
            where
                P_PARTKEY = PS_PARTKEY
                and S_SUPPKEY = PS_SUPPKEY
                and P_SIZE = {size}
                and S_NATIONKEY = {nation_key}
                and PS_SUPPLYCOST = (
                    select
                        min(PS_SUPPLYCOST)
                    from
                        PartSupp,
                        Supplier
                    where
                        P_PARTKEY = PS_PARTKEY
                        and S_SUPPKEY = PS_SUPPKEY
                        and S_NATIONKEY = {nation_key}
                )
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


class Query9:
    def __init__(self):
        ss = TPCDSStoreSales.load()
        duckdb.sql("CREATE TABLE store_sales AS SELECT * FROM ss")
        self.query_str = f"""
            select case when (select count(*) 
                              from store_sales 
                              where ss_quantity between 1 and 20) > 1071
                        then (select sum(ss_ext_tax) 
                              from store_sales 
                              where ss_quantity between 1 and 20) 
                        else (select sum(ss_net_paid_inc_tax)
                              from store_sales
                              where ss_quantity between 1 and 20) end bucket1 ,
                   case when (select count(*)
                              from store_sales
                              where ss_quantity between 21 and 40) > 39161
                        then (select sum(ss_ext_tax)
                              from store_sales
                              where ss_quantity between 21 and 40) 
                        else (select sum(ss_net_paid_inc_tax)
                              from store_sales
                              where ss_quantity between 21 and 40) end bucket2,
                   case when (select count(*)
                              from store_sales
                              where ss_quantity between 41 and 60) > 29434
                        then (select sum(ss_ext_tax)
                              from store_sales
                              where ss_quantity between 41 and 60)
                        else (select sum(ss_net_paid_inc_tax)
                              from store_sales
                              where ss_quantity between 41 and 60) end bucket3,
                   case when (select count(*)
                              from store_sales
                              where ss_quantity between 61 and 80) > 6568
                        then (select sum(ss_ext_tax)
                              from store_sales
                              where ss_quantity between 61 and 80)
                        else (select sum(ss_net_paid_inc_tax)
                              from store_sales
                              where ss_quantity between 61 and 80) end bucket4,
                   case when (select count(*)
                              from store_sales
                              where ss_quantity between 81 and 100) > 21216
                        then (select sum(ss_ext_tax)
                              from store_sales
                              where ss_quantity between 81 and 100)
                        else (select sum(ss_net_paid_inc_tax)
                              from store_sales
                              where ss_quantity between 81 and 100) end bucket5
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


class Query12:
    def __init__(self):
        lineitem = TPCHLineItem.load()
        orders = TPCHOrders.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        duckdb.sql("CREATE TABLE Orders AS SELECT * FROM orders")
        duckdb.sql("ALTER TABLE Orders ADD PRIMARY KEY (O_ORDERKEY);")
        self.query_str = f"""
            select
                l_orderkey,
                sum(case
                    when o_orderpriority = 0
                        or o_orderpriority = 1
                        then 1
                    else 0
                end) as high_line_count,
                sum(case
                    when o_orderpriority <> 0
                        and o_orderpriority <> 1
                        then 1
                    else 0
                end) as low_line_count
            from
                Orders,
                LineItem
            where
                o_orderkey = l_orderkey
                and l_commitdate < l_receiptdate
                and l_shipdate < l_commitdate
            group by
                l_orderkey
            order by
                l_orderkey
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


class Query15:
    def __init__(self):
        lineitem = TPCHLineItem.load()
        supplier = TPCHSupplier.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        duckdb.sql("CREATE TABLE Supplier AS SELECT * FROM supplier")
        duckdb.sql("ALTER TABLE Supplier ADD PRIMARY KEY (S_SUPPKEY);")
        self.query_str = f"""
        with revenue (supplier_no, total_revenue) as (
                select
                    l_suppkey,
                    sum(l_extendedprice * (1 - l_discount))
                from
                    LineItem
                where
                    L_SHIPDATE >= 820454400
                    and L_SHIPDATE < 852076800
                group by
                    l_suppkey
                )
        select
            s_suppkey,
            total_revenue
        from
            Supplier,
            revenue
        where
            s_suppkey = supplier_no
            and total_revenue = (
                select
                    max(total_revenue)
                from
                    revenue
            )
        order by
            s_suppkey
    """

    def run(self):
        return duckdb.sql(self.query_str).show()


class QueryExample:
    def __init__(self, nation_key=0, size=15):
        part = TPCHPart.load()
        supplier = TPCHSupplier.load()
        partsupp = TPCHPartSupp.load()
        duckdb.sql("CREATE TABLE Part AS SELECT * FROM part")
        duckdb.sql("ALTER TABLE Part ADD PRIMARY KEY (P_PARTKEY);")
        duckdb.sql("CREATE TABLE Supplier AS SELECT * FROM supplier")
        duckdb.sql("ALTER TABLE Supplier ADD PRIMARY KEY (S_SUPPKEY);")
        duckdb.sql("CREATE TABLE PartSupp AS SELECT * FROM partsupp")
        duckdb.sql("ALTER TABLE PartSupp ADD PRIMARY KEY (PS_PARTKEY, PS_SUPPKEY);")
        self.query_str = f"""
            select
                PS_PARTKEY,
                sum(PS_AVAILQTY)
            from
                PartSupp
            where
                ps_partkey in (
                    select
                        p_partkey
                    from
                        part
                    Where
                        p_size > {size}
                )
                and ps_suppkey in (
                    Select
                        s_suppkey
                    From
                        Supplier
                    Where
                        s_nationkey = {nation_key}
                )
            group by
                ps_partkey
            order by
                ps_partkey
        """
        query_str = f"""
            select
                PS_PARTKEY,
                sum(PS_AVAILQTY)
            from
                Part,
                PartSupp,
                supplier
            where
                ps_partkey = p_partkey
                and ps_suppkey = s_suppkey
                and p_size > {size}
                and s_nationkey = {nation_key}
            group by
                ps_partkey
            order by
                ps_partkey
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


class Query18:
    def __init__(self):
        lineitem = TPCHLineItem.load()
        orders = TPCHOrders.load()
        duckdb.sql("CREATE TABLE LineItem AS SELECT * FROM lineitem")
        duckdb.sql("ALTER TABLE LineItem ADD PRIMARY KEY (L_ORDERKEY, L_LINENUMBER);")
        duckdb.sql("CREATE TABLE Orders AS SELECT * FROM orders")
        duckdb.sql("ALTER TABLE Orders ADD PRIMARY KEY (O_ORDERKEY);")
        self.query_str = f"""
        select
            o_orderkey,
            sum(l_quantity)
        from
            Orders,
            LineItem
        where
            o_orderkey in (
                select
                    l_orderkey
                from
                    LineItem
                group by
                    l_orderkey having
                        sum(l_quantity) > 300
            )
            and o_orderkey = l_orderkey
        group by
            o_orderkey
        order by
            o_orderkey
        """

    def run(self):
        return duckdb.sql(self.query_str).show()


class DDSelectBench:
    def __init__(self):
        df = FakeData().load()
        df2 = FakeData().load()
        duckdb.sql("CREATE TABLE FakeData AS SELECT * FROM df")
        duckdb.sql("ALTER TABLE FakeData ADD PRIMARY KEY (t);")
        duckdb.sql("CREATE TABLE FakeData2 AS SELECT * FROM df2")
        duckdb.sql("ALTER TABLE FakeData2 ADD PRIMARY KEY (t);")

    def select(self):
        query_str = f"""
            CREATE TABLE out AS SELECT t FROM FakeData WHERE t%2 == 0;
        """
        return duckdb.sql(query_str)

    def ijoin(self):
        query_str = f"""
            CREATE TABLE out AS SELECT l.t, l.val - r.val FROM FakeData l JOIN FakeData2 r ON l.t = r.t;
        """
        return duckdb.sql(query_str)

    def ojoin(self):
        query_str = f"""
            CREATE TABLE out AS SELECT l.t, l.val - r.val FROM FakeData l FULL OUTER JOIN FakeData2 r ON l.t = r.t;
        """
        return duckdb.sql(query_str)

    def sum(self):
        query_str = f"""
            SELECT SUM(val) FROM FakeData;
        """
        return duckdb.sql(query_str).fetchone()


    def run(self, q):
        if q == "select":
            return self.select()
        elif q == "inner":
            return self.ijoin()
        elif q == "outer":
            return self.ojoin()
        elif q == "sum":
            return self.sum()


if __name__ == "__main__":
    q = DDSelectBench()
    import time
    start = time.time()
    out = q.run("sum")
    end = time.time()
    print(out)
    print("Time: ", (end - start) * 1000)
