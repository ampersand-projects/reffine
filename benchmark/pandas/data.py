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
        "L_LINENUMBER": np.int64,
        "L_PARTKEY": np.int64,
        "L_SUPPKEY": np.int64,
        "L_QUANTITY": np.float64,
        "L_EXTENDEDPRICE": np.float64,
        "L_DISCOUNT": np.float64,
        "L_TAX": np.float64,
        "L_RETURNFLAG": np.str_,
        "L_LINESTATUS": np.str_,
        "L_SHIPDATE": np.dtype('datetime64[s]'),
        "L_COMMITDATE": np.dtype('datetime64[s]'),
        "L_RECEIPTDATE": np.dtype('datetime64[s]'),
        "L_SHIPINSTRUCT": np.str_,
        "L_SHIPMODE": np.str_,
        "L_COMMENT": np.str_,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/lineitem.tbl",
            delimiter="|",
            engine="pyarrow",
            usecols = [0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
            names=list(cls.dtypes.keys()),
        ).astype(cls.dtypes).set_index(["L_ORDERKEY", "L_LINENUMBER"])
        df["L_SHIPDATE"] = df["L_SHIPDATE"].astype("int64")
        df["L_COMMITDATE"] = df["L_COMMITDATE"].astype("int64")
        df["L_RECEIPTDATE"] = df["L_RECEIPTDATE"].astype("int64")

        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
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
        ).astype(cls.dtypes).set_index(["C_CUSTKEY"])

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
        ).astype(cls.dtypes).set_index(["O_ORDERKEY"])
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
        ).fillna(value=0).astype(cls.dtypes).set_index(["ss_item_sk", "ss_ticket_number"])

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
        }).astype(cls.dtypes).set_index(["ts"])
        return df

    @classmethod
    def store(cls):
        df = cls.load().reset_index(drop=False)
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/stock_price.arrow", pa.table(cols))


class TPCHQuery6:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()

    def query(self, start_date, end_date, discount, quantity):
        filtered = self.lineitem[
            (lineitem["L_SHIPDATE"] >= start_date) &
            (lineitem["L_SHIPDATE"] < end_date) &
            (lineitem["L_DISCOUNT"].between(discount - 0.01, discount + 0.01)) &
            (lineitem["L_QUANTITY"] < quantity)
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

        return result

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

        return result

        
    def run(self):
        return self.query2(1, 795484800)


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

    def run(self):
        return self.query(700000000, 900000000)


class TPCDSQuery9:
    def __init__(self):
        self.store_sales = TPCDSStoreSales.load()

    def query(self):
        def bucket(df, low, high, threshold):
            f = df[(df['ss_quantity'] >= low) & (df['ss_quantity'] <= high)]
            if len(f) > threshold:
                return f['ss_ext_tax'].mean()
            else:
                return f['ss_net_paid_inc_tax'].mean()
        
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
        pass

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
        pairs["dist2"] = pairs["dx"]**2 + pairs["dy"]**2 + pairs["dx"]**2
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



#TPCHLineItem.store()
#TPCHCustomer.store()
#TPCHOrders.store()
#TPCHOrders.store()
#print(StockPrice.load())
#StockPrice.store()
q = NBody(2048)
#q.store()

import time
start = time.time()
res = q.run()
end = time.time()

print(res)
print("Time: ", end - start)


