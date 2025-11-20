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


class TPCHPart:
    dtypes = {
        "P_PARTKEY": np.int64,
        "P_NAME": np.str_,
        "P_MFGR": np.str_,
        "P_BRAND": np.int32,
        "P_TYPE": np.str_,
        "P_SIZE": np.int32,
        "P_CONTAINER": np.int32,
        "P_RETAILPRICE": np.float64,
        "P_COMMENT": np.str_,
    }

    brands = {
        "Brand#13": 0,
        "Brand#42": 1,
        "Brand#34": 2,
        "Brand#32": 3,
        "Brand#24": 4,
        "Brand#11": 5,
        "Brand#44": 6,
        "Brand#43": 7,
        "Brand#54": 8,
        "Brand#25": 9,
        "Brand#33": 10,
        "Brand#55": 11,
        "Brand#15": 12,
        "Brand#23": 13,
        "Brand#12": 14,
        "Brand#35": 15,
        "Brand#52": 16,
        "Brand#14": 17,
        "Brand#53": 18,
        "Brand#22": 19,
        "Brand#45": 20,
        "Brand#21": 21,
        "Brand#41": 22,
        "Brand#51": 23,
        "Brand#31": 24,
    }

    containers = {
        "JUMBO PKG": 0,
        "LG CASE": 1,
        "WRAP CASE": 2,
        "MED DRUM": 3,
        "SM PKG": 4,
        "MED BAG": 5,
        "SM BAG": 6,
        "LG DRUM": 7,
        "LG CAN": 8,
        "WRAP BOX": 9,
        "JUMBO CASE": 10,
        "JUMBO PACK": 11,
        "JUMBO BOX": 12,
        "MED PACK": 13,
        "LG BOX": 14,
        "JUMBO JAR": 15,
        "MED CASE": 16,
        "JUMBO BAG": 17,
        "SM CASE": 18,
        "MED PKG": 19,
        "LG BAG": 20,
        "LG PKG": 21,
        "JUMBO CAN": 22,
        "SM JAR": 23,
        "WRAP JAR": 24,
        "SM PACK": 25,
        "WRAP BAG": 26,
        "WRAP PKG": 27,
        "WRAP DRUM": 28,
        "LG PACK": 29,
        "MED CAN": 30,
        "SM BOX": 31,
        "LG JAR": 32,
        "SM CAN": 33,
        "WRAP PACK": 34,
        "MED JAR": 35,
        "WRAP CAN": 36,
        "SM DRUM": 37,
        "MED BOX": 38,
        "JUMBO DRUM": 39,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/part.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
            converters={
                "P_BRAND": lambda val : cls.brands[val],
                "P_CONTAINER": lambda val : cls.containers[val],
            },
        ).astype(cls.dtypes).set_index(["P_PARTKEY"])

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


class TPCHQuery17:
    def __init__(self):
        self.lineitem = TPCHLineItem.load().reset_index(drop=False)
        self.part = TPCHPart.load().reset_index(drop=False)

    def query(self, brand, container):
        part_f = self.part[
            (self.part["P_BRAND"] == brand) &
            (self.part["P_CONTAINER"] == container)
        ]

        lp = self.lineitem.merge(
            part_f,
            left_on="L_PARTKEY",
            right_on="P_PARTKEY",
            how="inner"
        )

        avg_qty = (
            self.lineitem
            .groupby("L_PARTKEY")["L_QUANTITY"]
            .mean()
            .mul(0.2)
            .rename("THRESH")
        )

        lp2 = lp.merge(
            avg_qty,
            left_on="L_PARTKEY",
            right_index=True,
            how="left"
        )

        lp_f = lp2[lp2["L_QUANTITY"] < lp2["THRESH"]]

        avg_yearly = lp_f["L_EXTENDEDPRICE"].sum() / 7.0

        return avg_yearly

    def run(self):
        return self.query(0, 0)


#TPCHLineItem.store()
#TPCHCustomer.store()
#TPCHOrders.store()
#TPCHPart.store()

import time
q = TPCHQuery17()
start = time.time()
res = q.run()
end = time.time()
print(res)
print("Time: ", end - start)
