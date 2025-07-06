import pyarrow as pa

def run_query(arr):
    total = 0
    print(arr)
    for n in arr['hours_studied']:
        total += n.as_py()

    print(total)

with pa.memory_map("students.arrow", "r") as src:
    students_array = pa.ipc.open_file(src).read_all()
    run_query(students_array)
