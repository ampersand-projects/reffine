import ir
import pyarrow as pa
import ctypes

# === 1. Your IR generation stays exactly the same ===

def transform_fn(n):
    vec_in_sym = ir._sym("x", ir._i64_t.ptr())
    vec_in_sym2 = ir._sym("z", ir._i64_t.ptr())
    vec_out_sym = ir._sym("y", ir._i64_t.ptr())

    length = ir._idx(n)
    idx_alloc = ir._alloc(ir._idx_t, ir._idx(1))
    idx_addr = ir._sym("idx_addr", idx_alloc)
    idx = ir._load(idx_addr, ir._idx(0))

    val_ptr = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_in_sym, idx])
    val = ir._load(val_ptr, ir._idx(0))

    val_ptr2 = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_in_sym2, idx])
    val2 = ir._load(val_ptr2, ir._idx(0))

    out_ptr = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_out_sym, idx])

    # loop = ir._loop(ir._load(vec_out_sym))
    loop = ir._loop(ir._load(vec_out_sym, ir._idx(0)))
    loop.init = ir._stmts([idx_alloc, ir._store(idx_addr, ir._idx(0), ir._idx(0))])
    loop.body = ir._stmts([
        ir._store(out_ptr, ir._add(ir._add(ir._i64(1), val), val2), ir._idx(0)),
        ir._store(idx_addr, ir._add(idx, ir._idx(1)), ir._idx(0)),
    ])
    loop.exit_cond = ir._gte(idx, length)

    loop_sym = ir._sym("loop", loop)

    fn = ir._func("foo", loop, [vec_out_sym, vec_in_sym, vec_in_sym2], {}, False)
    fn.insert_sym(idx_addr, idx_alloc)
    return fn


fn = transform_fn(100)
print(f"Example Function:\n{ir.to_string(fn)}")


# === 2. Optional: demonstrate the new ArrowTable interface ===
# (This part only applies if you want to run the vector ops from internal.cpp)

# Suppose you have a pyarrow.Table:
tbl = pa.table({"x": pa.array(range(100), pa.int64()),
                "z": pa.array(range(100, 200), pa.int64())})

# If you added the pybind helper from earlier (e.g. from_pyarrow_table)
# that returns a capsule containing ArrowTable*, use it here:
tbl_capsule = ir.from_pyarrow_table(tbl)

# You can now use your updated bindings:
print("Vector length:", ir.get_vector_len(tbl_capsule))
data_buf_addr = ir.get_vector_data_buf_addr(tbl_capsule, 0)
print("First element address:", hex(data_buf_addr))

# Read first element directly from buffer (for demo)
ptr_type = ctypes.POINTER(ctypes.c_int64)
arr_ptr = ctypes.cast(data_buf_addr, ptr_type)
print("First element value:", arr_ptr[0])
