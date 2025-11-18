import pyarrow as pa
import pyarrow.compute as pc

# Create a sample array with repeating values
x = [1, 1, 1, 2, 2, 3, 3, 3, 3, 4]
y = [1, 3, 7, 1, 9, 2, 3, 7, 8, 3]
d = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
xarr = pc.run_end_encode(pa.array(x))
yarr = pa.array(y)
darr = pa.array(d)

table = pa.Table.from_arrays([xarr, yarr, darr], names=["x", "y", "d"])

def write_to_file(table, file_name):
    with pa.OSFile(file_name, "wb") as sink:
        with pa.ipc.RecordBatchFileWriter(sink, table.schema) as writer:
            writer.write_table(table)

def read_from_file(file_name):
    with pa.ipc.open_file(file_name) as reader:
        restored_table = reader.read_all()
        return restored_table

file_name = "runend.arrow"
write_to_file(table, file_name)
print(read_from_file(file_name))
