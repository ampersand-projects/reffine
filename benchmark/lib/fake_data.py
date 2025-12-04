import numpy as np
import pandas as pd
import pyarrow as pa

def read_data(size):
    df = pd.DataFrame({
        't': range(size),
        'val': range(size),
    })
    return df


def write_data(filename, df):
    schema = pa.Schema.from_pandas(df, preserve_index=False)
    table = pa.Table.from_pandas(df, preserve_index=False)
    writer = pa.ipc.new_file(filename, schema)
    writer.write(table)
    writer.close()

out_file = "fake_data.arrow"
df = read_data(50_000_000)
write_data(out_file, df)
