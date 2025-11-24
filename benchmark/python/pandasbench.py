import scipy as sp
import networkx as nx
import numpy as np
import pandas as pd
import pyarrow as pa
import pyarrow.compute as pc

OUTPUT_DIR = "arrow_data"

def write_dataframe(filename, df):
    schema = pa.Schema.from_pandas(df, preserve_index=False)
    table = pa.Table.from_pandas(df, preserve_index=False)
    with pa.ipc.new_file(filename, schema) as writer:
        writer.write(table)

def write_table(filename, table):
    combined_table = table.combine_chunks()
    '''
    with pa.OSFile(filename, "wb") as sink:
        with pa.ipc.RecordBatchFileWriter(sink, combined_table.schema) as writer:
            writer.write_table(combined_table)
    '''
    with pa.ipc.new_file(filename, combined_table.schema) as writer:
        writer.write(combined_table)

def read_table(filename):
    with pa.ipc.open_file(filename) as reader:
        return reader.read_all()

class TPCHLineItem:
    dtypes = {
        "L_ORDERKEY": np.int64,
        "L_PARTKEY": np.int64,
        "L_SUPPKEY": np.int64,
        "L_LINENUMBER": np.int64,
        "L_QUANTITY": np.float64,
        "L_EXTENDEDPRICE": np.float64,
        "L_DISCOUNT": np.float64,
        "L_TAX": np.float64,
        "L_RETURNFLAG": np.int32,
        "L_LINESTATUS": np.str_,
        "L_SHIPDATE": np.dtype('datetime64[s]'),
        "L_COMMITDATE": np.dtype('datetime64[s]'),
        "L_RECEIPTDATE": np.dtype('datetime64[s]'),
        "L_SHIPINSTRUCT": np.str_,
        "L_SHIPMODE": np.str_,
        "L_COMMENT": np.str_,
    }

    arrow_dtypes = {
        "L_ORDERKEY": np.int64,
        "L_LINENUMBER": np.int64,
        "L_PARTKEY": np.int64,
        "L_SUPPKEY": np.int64,
        "L_QUANTITY": np.float64,
        "L_EXTENDEDPRICE": np.float64,
        "L_DISCOUNT": np.float64,
        "L_TAX": np.float64,
        "L_RETURNFLAG": np.int32,
        "L_LINESTATUS": np.str_,
        "L_SHIPDATE": np.dtype('datetime64[s]'),
        "L_COMMITDATE": np.dtype('datetime64[s]'),
        "L_RECEIPTDATE": np.dtype('datetime64[s]'),
        "L_SHIPINSTRUCT": np.str_,
        "L_SHIPMODE": np.str_,
        "L_COMMENT": np.str_,
    }

    ret_flag = {
        "A": 0,
        "N": 1,
        "R": 2,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/lineitem.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
            converters={"L_RETURNFLAG": lambda val : cls.ret_flag[val]},
        ).astype(cls.dtypes)
        df["L_SHIPDATE"] = df["L_SHIPDATE"].astype("int64")
        df["L_COMMITDATE"] = df["L_COMMITDATE"].astype("int64")
        df["L_RECEIPTDATE"] = df["L_RECEIPTDATE"].astype("int64")

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.arrow_dtypes.keys())}
        cols["L_ORDERKEY"] = pc.run_end_encode(cols["L_ORDERKEY"])
        write_table(OUTPUT_DIR + "/lineitem.arrow", pa.table(cols))


class TPCHCustomer:
    dtypes = {
        "C_CUSTKEY": np.int64,
        "C_NAME": np.str_,
        "C_ADDRESS": np.str_,
        "C_NATIONKEY": np.int64,
        "C_PHONE": np.str_,
        "C_ACCTBAL": np.float64,
        "C_MKTSEGMENT": np.int8,
        "C_COMMENT": np.str_,
    }

    mktseg_enum = {
        "BUILDING": 0,
        "AUTOMOBILE": 1,
        "MACHINERY": 2,
        "HOUSEHOLD": 3,
        "FURNITURE": 4,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/customer.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
            converters={"C_MKTSEGMENT": lambda val : cls.mktseg_enum[val]},
        ).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/customer.arrow", pa.table(cols))


class TPCHOrders:
    dtypes = {
        "O_ORDERKEY": np.int64,
        "O_CUSTKEY": np.int64,
        "O_ORDERSTATUS": np.str_,
        "O_TOTALPRICE": np.float64,
        "O_ORDERDATE": np.dtype('datetime64[s]'),
        "O_ORDERPRIORITY": np.int8,
        "O_CLERK": np.str_,
        "O_SHIPPRIORITY": np.int32,
        "O_COMMENT": np.str_,
    }

    orderpriority = {
        "1-URGENT": 0,
        "2-HIGH": 1,
        "3-MEDIUM": 2,
        "4-NOT SPECIFIED": 3,
        "5-LOW": 4,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/orders.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
            converters={"O_ORDERPRIORITY": lambda val : cls.orderpriority[val]},
        ).astype(cls.dtypes)
        df["O_ORDERDATE"] = df["O_ORDERDATE"].astype("int64")

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/orders.arrow", pa.table(cols))


class TPCDSStoreSales:
    dtypes = {
        "ss_item_sk": np.int64,
        "ss_ticket_number": np.int64,
        "ss_sold_date_sk": np.int64,
        "ss_sold_time_sk": np.int64,
        "ss_customer_sk": np.int64,
        "ss_cdemo_sk": np.int64,
        "ss_hdemo_sk": np.int64,
        "ss_addr_sk": np.int64,
        "ss_store_sk": np.int64,
        "ss_promo_sk": np.int64,
        "ss_quantity": np.int32,
        "ss_wholesale_cost": np.float64,
        "ss_list_price": np.float64,
        "ss_sales_price": np.float64,
        "ss_ext_discount_amt": np.float64,
        "ss_ext_sales_price": np.float64,
        "ss_ext_wholesale_cost": np.float64,
        "ss_ext_list_price": np.float64,
        "ss_ext_tax": np.float64,
        "ss_coupon_amt": np.float64,
        "ss_net_paid": np.float64,
        "ss_net_paid_inc_tax": np.float64,
        "ss_net_profit": np.float64,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpcds-v3.2.0rc2/tools/store_sales.tbl",
            delimiter="|",
            usecols = [2, 9, 0, 1, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22],
            names=list(cls.dtypes.keys()),
        ).fillna(value=0).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        cols["ss_item_sk"] = pc.run_end_encode(cols["ss_item_sk"])
        write_table(OUTPUT_DIR + "/store_sales.arrow", pa.table(cols))


class StockPrice:
    dtypes = {
        "ts": np.int64,
        "val": np.float32,
    }

    @classmethod
    def load(cls, size = 1000000):
        timestamps = np.arange(size, dtype=np.int64)
        values = np.random.randn(size) * 10 + 100

        df = pd.DataFrame({
            "ts": timestamps,
            "val": values,
        }).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/stock_price.arrow", pa.table(cols))


class TPCHPartSupp:
    dtypes = {
        "PS_PARTKEY": np.int64,
        "PS_SUPPKEY": np.int64,
        "PS_AVAILQTY": np.int32,
        "PS_SUPPLYCOST": np.float64,
        "PS_COMMENT": np.str_,
    }

    dtypes2 = {
        "PS_SUPPKEY": np.int64,
        "PS_PARTKEY": np.int64,
        "PS_AVAILQTY": np.int32,
        "PS_SUPPLYCOST": np.float64,
        "PS_COMMENT": np.str_,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/partsupp.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
        ).astype(cls.dtypes)

        return df

    @classmethod
    def load2(cls):
        df2 = cls.load()
        cs = df2.columns.tolist()
        cs[0], cs[1] = cs[1], cs[0]
        df2 = df2[cs]
        df2 = df2.sort_values(by=[cs[0], cs[1]])

        return df2

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        cols["PS_PARTKEY"] = pc.run_end_encode(cols["PS_PARTKEY"])
        write_table(OUTPUT_DIR + "/partsupp.arrow", pa.table(cols))

        df2 = cls.load2().reset_index(drop=False)
        cols2 = {key: pa.array(df2[key]) for key in list(cls.dtypes2.keys())}
        cols2["PS_SUPPKEY"] = pc.run_end_encode(cols2["PS_SUPPKEY"])
        write_table(OUTPUT_DIR + "/supppart.arrow", pa.table(cols2))


class TPCHSupplier:
    dtypes = {
        "S_SUPPKEY": np.int64,
        "S_NAME": np.str_,
        "S_ADDRESS": np.str_,
        "S_NATIONKEY": np.int64,
        "S_PHONE": np.str_,
        "S_ACCTBAL": np.float64,
        "S_COMMENT": np.str_,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/supplier.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
        ).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/supplier.arrow", pa.table(cols))


class TPCHPart:
    dtypes = {
        "P_PARTKEY": np.int64,
        "P_NAME": np.str_,
        "P_MFGR": np.str_,
        "P_BRAND": np.str_,
        "P_TYPE": np.str_,
        "P_SIZE": np.int32,
        "P_CONTAINER": np.str_,
        "P_RETAILPRICE": np.float64,
        "P_COMMENT": np.str_,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/part.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
        ).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/part.arrow", pa.table(cols))


class TPCHQuery6:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()

    def query(self, start_date, end_date, discount, quantity):
        filtered = self.lineitem[
            (self.lineitem["L_SHIPDATE"] >= start_date) &
            (self.lineitem["L_SHIPDATE"] < end_date) &
            (self.lineitem["L_DISCOUNT"].between(discount - 0.01, discount + 0.01)) &
            (self.lineitem["L_QUANTITY"] < quantity)
        ]
        revenue = (filtered["L_EXTENDEDPRICE"] * filtered["L_DISCOUNT"]).sum()

        return revenue;

    def run(self):
        return self.query(820454400, 852076800, 0.05, 24.5)


class TPCHQuery3:
    def __init__(self):
        self.lineitem = TPCHLineItem.load().reset_index(drop=False)
        self.customer = TPCHCustomer.load().reset_index(drop=False)
        self.orders = TPCHOrders.load().reset_index(drop=False)

    def query(self, segment, date):
        cust_f = self.customer[self.customer["C_MKTSEGMENT"] == segment]

        cust_orders = cust_f.merge(
            self.orders,
            left_on="C_CUSTKEY",
            right_on="O_CUSTKEY",
            how="inner",
        )
        cust_orders = cust_orders[cust_orders["O_ORDERDATE"] < date]

        joined = cust_orders.merge(
            self.lineitem,
            left_on="O_ORDERKEY",
            right_on="L_ORDERKEY",
            how="inner",
        )
        
        joined = joined[joined["L_SHIPDATE"] > date]
        
        result = (
            joined
            .assign(REVENUE=lambda df: df["L_EXTENDEDPRICE"] * (1 - df["L_DISCOUNT"]))
            .groupby("L_ORDERKEY", as_index=False)["REVENUE"]
            .sum()
        )

        return result[:100]

    def query2(self, segment, date):
        joined = self.lineitem.merge(self.orders, left_on="L_ORDERKEY", right_on="O_ORDERKEY", how="inner")
        cust_joined = joined.merge(self.customer, left_on="O_CUSTKEY", right_on="C_CUSTKEY", how="inner")
        filtered = cust_joined[
            (cust_joined["C_MKTSEGMENT"] == segment) &
            (cust_joined["O_ORDERDATE"] < date) &
            (cust_joined["L_SHIPDATE"] > date)
        ]

        result = (
            filtered
            .assign(REVENUE=lambda df: df["L_EXTENDEDPRICE"] * (1 - df["L_DISCOUNT"]))
            .groupby("L_ORDERKEY", as_index=False)["REVENUE"]
            .sum()
        )

        return result[:100]

        
    def run(self):
        return self.query(1, 795484800)


class TPCHQuery4:
    def __init__(self):
        self.lineitem = TPCHLineItem.load().reset_index(drop=False)
        self.orders = TPCHOrders.load().reset_index(drop=False)

    def query(self, start_date, end_date):
        orders_f = self.orders[
            (self.orders["O_ORDERDATE"] >= start_date) &
            (self.orders["O_ORDERDATE"] < end_date)
        ]

        lineitem_f = self.lineitem[
            self.lineitem["L_COMMITDATE"] < self.lineitem["L_RECEIPTDATE"]
        ]

        valid_orderkeys = set(lineitem_f["L_ORDERKEY"].unique())

        orders_exists = orders_f[
            orders_f["O_ORDERKEY"].isin(valid_orderkeys)
        ]

        result = (
            orders_exists
            .groupby("O_ORDERPRIORITY")
            .count()
        )

        return result

    def query2(self, start_date, end_date):
        jn = self.lineitem.merge(self.orders, left_on="L_ORDERKEY", right_on="O_ORDERKEY")

        jn = jn[(jn["O_ORDERDATE"] >= start_date) & (jn["O_ORDERDATE"] < end_date)]
        jn = jn[jn["L_COMMITDATE"] < jn["L_RECEIPTDATE"]]

        jn = jn.drop_duplicates(subset=["O_ORDERPRIORITY", "L_ORDERKEY"])

        gb = jn.groupby("O_ORDERPRIORITY", as_index=False)
        agg = gb.agg(order_count=pd.NamedAgg(column="O_ORDERKEY", aggfunc="count"))

        return agg


    def run(self):
        return self.query(700000000, 900000000)


class TPCDSQuery9:
    def __init__(self):
        self.store_sales = TPCDSStoreSales.load()

    def query(self):
        def bucket(df, low, high, threshold):
            f = df[(df['ss_quantity'] >= low) & (df['ss_quantity'] <= high)]
            if len(f) > threshold:
                return f['ss_ext_tax'].sum()
            else:
                return f['ss_net_paid_inc_tax'].sum()
        
        bucket1 = bucket(self.store_sales, 1, 20, 1071)
        bucket2 = bucket(self.store_sales, 21, 40, 39161)
        bucket3 = bucket(self.store_sales, 41, 60, 29434)
        bucket4 = bucket(self.store_sales, 61, 80, 6568)
        bucket5 = bucket(self.store_sales, 81, 100, 21216)
        
        result = pd.DataFrame([{
            'bucket1': bucket1,
            'bucket2': bucket2,
            'bucket3': bucket3,
            'bucket4': bucket4,
            'bucket5': bucket5
        }])

        return result

    def run(self):
        return self.query()


class AlgoTrading:
    def __init__(self):
        self.stock_price = StockPrice.load()

    def query(self):
        df = self.stock_price

        df['price_10'] = df['val'].shift(10)
        df['price_20'] = df['val'].shift(20)
    
        # Basic differences
        df['diff_10'] = df['val'] - df['price_10']
        df['diff_20'] = df['val'] - df['price_20']
    
        # Your requirement: subtract 10-step value from 20-step value
        df['past_diff'] = df['price_20'] - df['price_10']
    
        return df[["val", "past_diff"]]

    def run(self):
        return self.query()


class NBody:
    def __init__(self, n):
        self.bodies = self.make_nbody_df(n)

    def make_nbody_df(self, n):
        df = pd.DataFrame({
            "ID": np.arange(n, dtype=np.int64),
            "M": np.random.uniform(1.0, 10.0, size=n),
            "X": np.random.uniform(-1, 1, size=n),
            "Y": np.random.uniform(-1, 1, size=n),
            "Z": np.random.uniform(-1, 1, size=n),
            "VX": np.random.uniform(-0.1, 0.1, size=n),
            "VY": np.random.uniform(-0.1, 0.1, size=n),
            "VZ": np.random.uniform(-0.1, 0.1, size=n),
        })

        return df

    def query(self, df, G=1.0, dt=0.01):
        # 1. Cartesian product (each body interacts with every other)
        pairs = df.merge(df, how="cross", suffixes=("_A", "_B"))
        pairs = pairs[pairs["ID_A"] != pairs["ID_B"]]

        # 2. Compute displacements and distances
        pairs["dx"] = pairs["X_B"] - pairs["X_A"]
        pairs["dy"] = pairs["Y_B"] - pairs["Y_A"]
        pairs["dz"] = pairs["Z_B"] - pairs["Z_A"]
        pairs["dist2"] = pairs["dx"]**2 + pairs["dy"]**2 + pairs["dz"]**2
        pairs["dist"] = np.sqrt(pairs["dist2"])

        # 3. Newtonian gravitational force magnitude
        pairs["F"] = G * (pairs["M_A"] * pairs["M_B"]) / pairs["dist2"]

        # 4. Force components (normalize direction)
        pairs["Fx"] = pairs["F"] * pairs["dx"] / pairs["dist"]
        pairs["Fy"] = pairs["F"] * pairs["dy"] / pairs["dist"]
        pairs["Fz"] = pairs["F"] * pairs["dz"] / pairs["dist"]

        # 5. Total force on each body
        forces = pairs.groupby("ID_A")[["Fx", "Fy", "Fz"]].sum()

        # 6. Update velocities and positions
        df = df.set_index("ID").copy()
        df["VX"] += (forces["Fx"] / df["M"]) * dt
        df["VY"] += (forces["Fy"] / df["M"]) * dt
        df["VZ"] += (forces["Fz"] / df["M"]) * dt

        df["X"] += df["VX"] * dt
        df["Y"] += df["VY"] * dt
        df["Z"] += df["VZ"] * dt

        return df.reset_index()

    def store(self):
        df = self.bodies
        write_table(OUTPUT_DIR + "/bodies.arrow", pa.Table.from_pandas(self.bodies))

    def run(self):
        df = self.bodies
        return self.query(df)
        for i in range(100):
            df = self.query(df)
        return df


class PageRank:
    def __init__(self):
        edges = pd.read_csv(
            "./lib/snap/twitter_combined.tbl",
            #"./lib/snap/email-Eu-core.txt",
            sep=" ",
            comment="#",
            header=None,
            names=["src", "dst"],
        )
        self.edges = edges.groupby(["src", "dst"]).size().reset_index(drop=False, name="count").sort_values(["src", "dst"])
        self.rev_edges = edges.groupby(["dst", "src"]).size().reset_index(drop=False, name="count").sort_values(["dst", "src"])
        nodes = pd.Index(sorted(set(self.edges["src"]).union(self.edges["dst"])))
        self.N = len(nodes)
        self.pr = pd.Series(1.0 / self.N, index=nodes).reset_index(drop=False)
        self.pr.columns = ["node", "pr"]

        self.G = nx.read_edgelist(
            "lib/snap/twitter_combined.tbl",
            create_using=nx.DiGraph(),   # PageRank requires directed graph
            nodetype=int                 # nodes are integers in the SNAP dataset
        )
        N = len(self.G)
        nodelist = list(self.G)
        self.A = nx.to_scipy_sparse_array(self.G, nodelist=nodelist, weight="weight", dtype=float)


    def store(self):
        table = pa.Table.from_pandas(self.edges)
        src = table.column(table.column_names[0])
        dst = table.column(table.column_names[1])
        count = table.column(table.column_names[2])
        src = pc.run_end_encode(src)
        tbl = pa.table({"src": src, "dst": dst, "count": count})
        write_table(OUTPUT_DIR + "/edges.arrow", tbl)

        table2 = pa.Table.from_pandas(self.rev_edges)
        dst2 = table2.column(table2.column_names[0])
        src2 = table2.column(table2.column_names[1])
        count2 = table2.column(table2.column_names[2])
        dst2 = pc.run_end_encode(dst2)
        tbl2 = pa.table({"dst": dst2, "src": src2, "count": count2})
        write_table(OUTPUT_DIR + "/rev_edges.arrow", tbl2)

        tbl_pr = pa.Table.from_pandas(self.pr)
        write_table(OUTPUT_DIR + "/pr.arrow", tbl_pr)

    def query(self, edges, pr, N, alpha=0.85):
        pr = pr.set_index("node")

        # Compute outdegree
        outdeg = edges.groupby("src").size().rename("outdeg")

        # Join outdegree onto edges
        e = edges.join(outdeg, on="src")

        # Contribution of each edge = PR(src) / outdeg(src)
        e["contrib"] = pr["pr"][e["src"]].values / e["outdeg"].values

        # Sum contributions for each destination node
        new_pr = e.groupby("dst")["contrib"].sum()

        # Teleportation + damping
        N = len(pr)
        new_pr = alpha * new_pr
        new_pr += (1 - alpha) / N

        new_pr = new_pr.reindex(pr.index, fill_value=(1 - alpha) / N)
        return new_pr

    def google_matrix(self,
        G, alpha=0.85, personalization=None, nodelist=None, weight="weight", dangling=None
    ):
        if nodelist is None:
            nodelist = list(G)
    
        A = nx.to_numpy_array(G, nodelist=nodelist, weight=weight)
        N = len(G)
        if N == 0:
            return A
    
        # Personalization vector
        if personalization is None:
            p = np.repeat(1.0 / N, N)
        else:
            p = np.array([personalization.get(n, 0) for n in nodelist], dtype=float)
            if p.sum() == 0:
                raise ZeroDivisionError
            p /= p.sum()
    
        # Dangling nodes
        if dangling is None:
            dangling_weights = p
        else:
            # Convert the dangling dictionary into an array in nodelist order
            dangling_weights = np.array([dangling.get(n, 0) for n in nodelist], dtype=float)
            dangling_weights /= dangling_weights.sum()
        dangling_nodes = np.where(A.sum(axis=1) == 0)[0]
    
        # Assign dangling_weights to any dangling nodes (nodes with no out links)
        A[dangling_nodes] = dangling_weights
    
        A /= A.sum(axis=1)[:, np.newaxis]  # Normalize rows to sum to 1
    
        return alpha * A + (1 - alpha) * p

    def _pagerank_scipy(
        self,
        G,
        alpha=0.85,
        personalization=None,
        max_iter=100,
        tol=1.0e-6,
        nstart=None,
        weight="weight",
        dangling=None,
    ):
        N = len(G)
        '''
        nodelist = list(G)
        A = nx.to_scipy_sparse_array(G, nodelist=nodelist, weight=weight, dtype=float)
        '''
        A = self.A
        S = A.sum(axis=1)
        S[S != 0] = 1.0 / S[S != 0]
        Q = sp.sparse.dia_array((S.T, 0), shape=A.shape).tocsr()
        A = Q @ A
    
        x = np.repeat(1.0 / N, N)
        p = np.repeat(1.0 / N, N)
        x = (alpha * (x @ A)) + (1 - alpha) * p

    def run(self):
        return self.query(self.edges, self.pr, self.N)
        #return self.google_matrix(self.G)


class TPCHQuery11:
    def __init__(self):
        self.supplier = TPCHSupplier.load().reset_index(drop=False)
        self.partsupp = TPCHPartSupp.load().reset_index(drop=False)
        self.supppart = TPCHPartSupp.load2().reset_index(drop=False)

    def query(self, nation_key, fraction):
        supp_nat = self.supplier[self.supplier["S_NATIONKEY"] == nation_key]

        ps = self.partsupp.merge(
            supp_nat,
            left_on="PS_SUPPKEY",
            right_on="S_SUPPKEY",
            how="inner"
        )

        ps["VALUE"] = ps["PS_SUPPLYCOST"] * ps["PS_AVAILQTY"]

        per_part = (
            ps.groupby("PS_PARTKEY", as_index=False)["VALUE"]
              .sum()
              .rename(columns={"VALUE": "VALUE"})
        )

        total_value = ps["VALUE"].sum()

        threshold = total_value * fraction

        result = per_part[per_part["VALUE"] > threshold]

        return result

    def run(self):
        return self.query(0, 0.0001)


class TPCHQuery1:
    def __init__(self):
        self.lineitem = TPCHLineItem.load().reset_index(drop=False)

    def query(self):
        var1 = 904694400

        filt = self.lineitem[self.lineitem["L_SHIPDATE"] <= var1]

        # This is lenient towards pandas as normally an optimizer should decide
        # that this could be computed before the groupby aggregation.
        # Other implementations don't enjoy this benefit.
        filt["disc_price"] = filt.L_EXTENDEDPRICE * (1.0 - filt.L_DISCOUNT)
        filt["charge"] = (
            filt.L_EXTENDEDPRICE * (1.0 - filt.L_DISCOUNT) * (1.0 + filt.L_TAX)
        )

        gb = filt.groupby(["L_RETURNFLAG"], as_index=False)
        agg = gb.agg(
            sum_qty=pd.NamedAgg(column="L_QUANTITY", aggfunc="sum"),
            sum_base_price=pd.NamedAgg(column="L_EXTENDEDPRICE", aggfunc="sum"),
            sum_disc_price=pd.NamedAgg(column="disc_price", aggfunc="sum"),
            sum_charge=pd.NamedAgg(column="charge", aggfunc="sum"),
            count_order=pd.NamedAgg(column="L_ORDERKEY", aggfunc="size"),
        )

        return agg

    def run(self):
        return self.query()


class TPCHQuery2:
    def __init__(self):
        self.supplier = TPCHSupplier.load().reset_index(drop=False)
        self.partsupp = TPCHPartSupp.load().reset_index(drop=False)
        self.supppart = TPCHPartSupp.load2().reset_index(drop=False)

    def query(self, nation_key, fraction):
        supp_nat = self.supplier[self.supplier["S_NATIONKEY"] == nation_key]

        ps = self.partsupp.merge(
            supp_nat,
            left_on="PS_SUPPKEY",
            right_on="S_SUPPKEY",
            how="inner"
        )

        ps["VALUE"] = ps["PS_SUPPLYCOST"] * ps["PS_AVAILQTY"]

        per_part = (
            ps.groupby("PS_PARTKEY", as_index=False)["VALUE"]
              .sum()
              .rename(columns={"VALUE": "VALUE"})
        )

        total_value = ps["VALUE"].sum()

        threshold = total_value * fraction

        result = per_part[per_part["VALUE"] > threshold]

        return result

    def run(self):
        return self.query(0, 0.0001)

if __name__ == '__main__':
    q = TPCHQuery1()
    import time
    start = time.time()
    out = q.run()
    end = time.time()
    print(out)
    print("Time: ", (end - start)*1000)


