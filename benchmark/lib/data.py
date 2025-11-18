import numpy as np
import pandas as pd
import pyarrow as pa
import pyarrow.compute as pc

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

def tpch_query6():
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
    
    df = pd.read_csv(
        "tpch-v3.0.1/dbgen/lineitem.tbl",
        delimiter="|",
        engine="pyarrow",
        usecols = [0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
        names=list(dtypes.keys()),
    ).astype(dtypes)
    df["L_SHIPDATE"] = df["L_SHIPDATE"].astype("int64")
    df["L_COMMITDATE"] = df["L_COMMITDATE"].astype("int64")
    df["L_RECEIPTDATE"] = df["L_RECEIPTDATE"].astype("int64")
    
    cols = {key: pa.array(df[key]) for key in list(dtypes.keys())}
    cols["L_ORDERKEY"] = pc.run_end_encode(cols["L_ORDERKEY"])
    
    tbl = pa.table(cols)
    write_table("lineitem.arrow", tbl)

tpch_query6()
print(read_table("lineitem.arrow"))
