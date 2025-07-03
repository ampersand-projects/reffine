import ir
import exec
import numpy as np
import pyarrow as pa

def transform_fn(n):
    vec_in_sym = ir._sym("x", ir._i64_t.ptr())
    vec_in_sym2 = ir._sym("y", ir._i64_t.ptr())
    vec_out_sym = ir._sym("z", ir._i64_t.ptr())

    length = ir._idx(n)
    idx_alloc = ir._alloc(ir._idx_t, ir._u32(1))
    idx_addr = ir._sym("idx_addr", idx_alloc)
    idx = ir._load(idx_addr)

    val_ptr = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_in_sym, idx])
    val = ir._load(val_ptr)
    
    val_ptr2 = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_in_sym2, idx])
    val2 = ir._load(val_ptr2)

    out_ptr = ir._call("get_elem_ptr", ir._i64_t.ptr(), [vec_out_sym, idx])

    loop = ir._loop(ir._load(vec_out_sym))
    loop.init = ir._stmts([idx_alloc, ir._store(idx_addr, ir._idx(0))])
    loop.body = ir._stmts([
        ir._store(out_ptr, ir._add(val, val2)),
        ir._store(idx_addr, ir._add(idx, ir._idx(1))),
    ])
    loop.exit_cond = ir._gte(idx, length)

    loop_sym = ir._sym("loop", loop)

    fn = ir._func("foo", loop, [vec_out_sym, vec_in_sym, vec_in_sym2], {}, False)
    fn.insert_sym(idx_addr, idx_alloc)
    
    return fn


def test_numpy():
    fn = transform_fn(100)
    print(f"Loop IR:\n{exec.to_string(fn)}\n\n")

    out_arr = np.ones(100, dtype=np.int64)
    in_arr = np.arange(100, 200, dtype=np.int64)
    in_arr2 = np.arange(100, dtype=np.int64)
    
    print(f"Input 1:\n{in_arr}\n\n")
    print(f"Input 2:\n{in_arr2}\n\n")
    
    query = exec.compile_loop(fn)
    print("Executing query...\n")
    print(f"Output:\n{exec.execute_query(query, out_arr, [in_arr, in_arr2])}\n\n")

    
def transform_fn_arrow(n=100):
    vec_out_sym = ir._sym("out", ir.VECTOR(1, [ir._i64_t]))
    vec_sym = ir._sym("in", ir.VECTOR(1, [ir._i64_t, ir._i64_t]))

    length = ir._call("get_vector_len", ir._idx_t, [vec_sym])

    length = ir._idx(n)
    idx_alloc = ir._alloc(ir._idx_t, ir._u32(1))
    idx_addr = ir._sym("idx_addr", idx_alloc)
    idx = ir._load(idx_addr)

    val_ptr = ir._fetch(vec_sym, idx, 0)
    val = ir._load(val_ptr)
    
    val_ptr2 = ir._fetch(vec_sym, idx, 1)
    val2 = ir._load(val_ptr2)

    out_ptr = ir._fetch(vec_out_sym, idx, 0)

    loop = ir._loop(vec_out_sym)
    loop.init = ir._stmts([idx_alloc, ir._store(idx_addr, ir._idx(0))])
    loop.body = ir._stmts([
        ir._store(out_ptr, ir._add(val, val2)),
        ir._store(idx_addr, ir._add(idx, ir._idx(1))),
    ])
    loop.exit_cond = ir._gte(idx, length)

    loop_sym = ir._sym("loop", loop)

    fn = ir._func("foo", loop, [vec_out_sym, vec_sym], {}, False)
    fn.insert_sym(idx_addr, idx_alloc)
    
    return fn

    
def test_arrow():
    in_arr = pa.StructArray.from_arrays([
        pa.array([x for x in range(100)]),
        pa.array([x for x in range(100, 200)])
    ], names=['x', 'y'])
    
    print(in_arr)

    out_arr = pa.StructArray.from_arrays([
        pa.array([0 for x in range(100)])
    ], names=['z'])

    _, c_in_arr = in_arr.__arrow_c_array__()
    _, c_out_arr = out_arr.__arrow_c_array__()
    
    fn = transform_fn_arrow(100)
    print(f"Loop IR:\n{exec.to_string(fn)}\n\n")
    
    query = exec.compile_loop(fn)    
    print("Executing query...\n")
    exec.execute_query(query, c_out_arr, [c_in_arr])
    print(f"Output:\n{out_arr}\n\n")

test_arrow()
# test_numpy()
