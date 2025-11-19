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
        ).astype(cls.dtypes)
        df["L_SHIPDATE"] = df["L_SHIPDATE"].astype("int64")
        df["L_COMMITDATE"] = df["L_COMMITDATE"].astype("int64")
        df["L_RECEIPTDATE"] = df["L_RECEIPTDATE"].astype("int64")

        return df

    @classmethod
    def store(cls):
        df = cls.load()
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
        ).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load()
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/customer.arrow", pa.table(cols))


class TPCHOrders:
    dtypes = {
        "O_ORDERKEY": np.int64,
        "O_CUSTKEY": np.int64,
        "O_ORDERSTATUS": np.str_,
        "O_TOTALPRICE": np.float64,
        "O_ORDERDATE": np.str_,
        "O_ORDERPRIORITY": np.str_,
        "O_CLERK": np.str_,
        "O_SHIPPRIORITY": np.int32,
        "O_COMMENT": np.str_,
    }

    @classmethod
    def load(cls):
        df = pd.read_csv(
            "lib/tpch-v3.0.1/dbgen/orders.tbl",
            delimiter="|",
            names=list(cls.dtypes.keys()),
        ).astype(cls.dtypes)

        return df

    @classmethod
    def store(cls):
        df = cls.load()
        cols = {key: pa.array(df[key]) for key in list(cls.dtypes.keys())}
        write_table(OUTPUT_DIR + "/orders.arrow", pa.table(cols))


class TPCHQuery6:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()

    @classmethod
    def tpch_query6(cls, lineitem, start_date, end_date, discount, quantity):
        filtered = lineitem[
            (lineitem["L_SHIPDATE"] >= start_date) &
            (lineitem["L_SHIPDATE"] < end_date) &
            (lineitem["L_DISCOUNT"].between(discount - 0.01, discount + 0.01)) &
            (lineitem["L_QUANTITY"] < quantity)
        ]
        revenue = (filtered["L_EXTENDEDPRICE"] * filtered["L_DISCOUNT"]).sum()

        return revenue;

    def run(self):
        return self.tpch_query6(self.lineitem, 820454400, 852076800, 0.05, 24.5)


class TPCHQuery3:
    def __init__(self):
        self.lineitem = TPCHLineItem.load()
        self.customer = TPCHCustomer.load()

TPCHLineItem.store()
TPCHCustomer.store()
TPCHOrders.store()
