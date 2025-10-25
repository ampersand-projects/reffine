import pyarrow as pa
import pyarrow.compute as pc

# Create a sample array with repeating values
t = [1, 1, 1, 2, 2, 3, 3, 3, 3, 4]
d = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
tarr = pc.run_end_encode(pa.array(t))
darr = pa.array(d)

# Print the run-end encoded array
print(tarr)
print(darr)

table = pa.Table.from_arrays([tarr, darr], names=["t", "d"])

def write_to_file(table, file_name):
    with pa.OSFile(file_name, "wb") as sink:
        with pa.ipc.RecordBatchFileWriter(sink, table.schema) as writer:
            writer.write_table(table)

def read_from_file(file_name):
    with pa.ipc.open_file(file_name) as reader:
        restored_table = reader.read_all()
        return restored_table

write_to_file(table, "data.arrow")
print(read_from_file("data.arrow"))
