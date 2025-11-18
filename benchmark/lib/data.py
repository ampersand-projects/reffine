import numpy as np
import pandas as pd
import pyarrow as pa

def write_arrow(filename, df):
    schema = pa.Schema.from_pandas(df, preserve_index=False)
    table = pa.Table.from_pandas(df, preserve_index=False)
    with pa.ipc.new_file(filename, schema) as writer:
        writer.write(table)

dtypes = {
    "L_ORDERKEY": np.int64,
    "L_PARTKEY": np.int64,
    "L_SUPPKEY": np.int64,
    "L_LINENUMBER": np.int64,
    "L_QUANTITY": np.float64,
    "L_EXTENDEDPRICE": np.float64,
    "L_DISCOUNT": np.float64,
    "L_TAX": np.float64,
    "L_RETURNFLAG": np.bytes_,
    "L_LINESTATUS": np.bytes_,
    "L_SHIPDATE": np.datetime64,
    "L_COMMITDATE": np.datetime64,
    "L_RECEIPTDATE": np.datetime64,
    "L_SHIPINSTRUCT": np.bytes_,
    "L_SHIPMODE": np.bytes_,
    "L_COMMENT": np.bytes_,
}

df = pd.read_csv(
    "tpch-v3.0.1/dbgen/lineitem.tbl",
    delimiter="|",
    header=None,
    engine="pyarrow",
    sep=r"|\s*"
    #names=list(dtypes.keys())
)
#).astype(dtypes)
print(df)
print(df.dtypes)
#write_arrow(out_file, df)
