import numpy as np
import pandas as pd
import pyarrow as pa

dtypes = {
    "id": np.int64,
    "hours_studied": np.int64,
    "attendance": np.int64,
    "sleep_hours": np.int64,
    "previous_scores": np.int64,
    "exam_score": np.int64,
}

def run_query(arr):
    total = 0
    print(arr)
    for n in arr['hours_studied']:
        total += n.as_py()

    print(total)

with pa.memory_map("students.arrow", "r") as src:
    students_array = pa.ipc.open_file(src).read_all()
    run_query(students_array)

def read_data(filename):
    df = pd.read_csv(
        filename,
        delimiter=",",
        engine="pyarrow",
    )
    return df[list(dtypes.keys())]

def write_data(filename, df):
    schema = pa.Schema.from_pandas(df, preserve_index=False)
    table = pa.Table.from_pandas(df, preserve_index=False)
    writer = pa.ipc.new_file(filename, schema)
    writer.write(table)
    writer.close()

in_file = "students.csv"
out_file = "students.arrow"
df = read_data(in_file)
write_data(out_file, df)
