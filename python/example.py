import ir
import exec
import numpy as np

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
print(f"Loop IR:\n{exec.to_string(fn)}\n\n")

out_arr = np.zeros(100, dtype=np.int64)
in_arr = np.arange(100, dtype=np.int64)
in_arr2 = np.arange(100, dtype=np.int64)
print(f"Loop execution:\n{exec.execute_loop(fn, out_arr, [in_arr, in_arr2])}\n\n")
