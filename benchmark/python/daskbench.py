from pandasbench import *
import dask
import dask.dataframe as dd

NTHREADS=8
dask.config.set(scheduler="threads", num_workers=1, num_threads=NTHREADS)

class Query6:
    def __init__(self):
        pdf = TPCHLineItem.load()
        self.lineitem = dd.from_pandas(pdf, npartitions=NTHREADS) 

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


class Query3:
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


class Query4:
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

class DaskAlgoTrading:
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


class DaskNBody:
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


class DaskPageRank:
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


class Query11:
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


class Query1:
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


class Query2:
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
    q = Query6()
    import time
    start = time.time()
    out = q.run().compute()
    end = time.time()
    print(out)
    print("Time: ", (end - start)*1000)
