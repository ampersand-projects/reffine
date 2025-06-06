"""
Correct approach: Wrap PyArrow capsules in ArrowTable structures
before passing to compiled code.
"""

import pyarrow as pa
import pyreffine.ir as ir
import pyreffine.exec as exec


def transform_fn_arrow(n=100):
    """Build the IR for vector addition"""
    vec_out_sym = ir._sym("out", ir.VECTOR(1, [ir._i64_t]))
    vec_sym = ir._sym("in", ir.VECTOR(2, [ir._i64_t, ir._i64_t]))

    length = ir._idx(n)
    idx_alloc = ir._alloc(ir._idx_t, ir._idx(1))
    idx_addr = ir._sym("idx_addr", idx_alloc)
    idx = ir._load(idx_addr, ir._idx(0))

    # Fetch column 0 (x)
    val_ptr = ir._fetch(vec_sym, 0)
    val = ir._load(val_ptr, idx)
    
    # Fetch column 1 (y)
    val_ptr2 = ir._fetch(vec_sym, 1)
    val2 = ir._load(val_ptr2, idx)

    # Output column
    out_ptr = ir._fetch(vec_out_sym, 0)

    loop = ir._loop(vec_out_sym)
    loop.init = ir._stmts([idx_alloc, ir._store(idx_addr, ir._idx(0), ir._idx(0))])
    loop.body = ir._stmts([
        ir._store(out_ptr, ir._add(val, val2), idx),
        ir._store(idx_addr, ir._add(idx, ir._idx(1)), ir._idx(0)),
    ])
    loop.exit_cond = ir._gte(idx, length)

    fn = ir._func("foo", loop, [vec_out_sym, vec_sym], {}, False)
    fn.insert_sym(idx_addr, idx_alloc)
    
    return fn


def test_with_pyarrow_direct():
    """
    The key insight: We need to wrap PyArrow capsules in ArrowTable
    before passing to the compiled function.
    """
    
    # Create PyArrow data
    in_arr = pa.StructArray.from_arrays([
        pa.array(list(range(100)), type=pa.int64()),
        pa.array(list(range(100, 200)), type=pa.int64())
    ], names=['x', 'y'])
    
    out_arr = pa.StructArray.from_arrays([
        pa.array([0] * 100, type=pa.int64())
    ], names=['z'])
    
    print(f"Input (first 5):")
    print(f"  x: {in_arr.field('x')[:5].to_pylist()}")
    print(f"  y: {in_arr.field('y')[:5].to_pylist()}\n")
    
    # Get both schema and array capsules
    in_schema_cap, in_array_cap = in_arr.__arrow_c_array__()
    out_schema_cap, out_array_cap = out_arr.__arrow_c_array__()
    
    # Create ArrowTable wrappers using from_arrow
    print("Creating ArrowTable wrappers...")
    in_table = ir.ArrowTable.from_arrow(in_schema_cap, in_array_cap)
    out_table = ir.ArrowTable.from_arrow(out_schema_cap, out_array_cap)
    
    print(f"  Input table created: {type(in_table)}")
    print(f"  Output table created: {type(out_table)}\n")
    
    # Build and compile
    fn = transform_fn_arrow(100)
    print(f"Function IR:\n{exec.to_string(fn)}\n")
    
    query = exec.compile_loop(fn)
    print("Executing query with ArrowTable wrappers...\n")
    
    # Execute - this should work now because we're passing ArrowTable* not ArrowArray*
    exec.run(query, [out_table, in_table])
    
    print(f"Output (first 5): {out_arr.field('z')[:5].to_pylist()}")
    print(f"Expected: [100, 102, 104, 106, 108]")
    
    # Verify results
    expected = [x + (x + 100) for x in range(100)]
    actual = out_arr.field('z').to_pylist()
    
    if actual == expected:
        print("\n✓ SUCCESS: Results match expected values!")
    else:
        print("\n✗ FAILURE: Results don't match")
        print(f"  First mismatch at index: {next(i for i, (a, e) in enumerate(zip(actual, expected)) if a != e)}")


def test_native_arrowtable():
    """
    Alternative: Create ArrowTable directly in C++ (if you populate data)
    """
    print("Creating native ArrowTable objects...\n")
    
    # Create tables
    in_table = ir.ArrowTable(
        "input",
        100,
        ["x", "y"],
        [ir._i64_t, ir._i64_t]
    )
    
    out_table = ir.ArrowTable(
        "output",
        100,
        ["z"],
        [ir._i64_t]
    )
    
    print(f"  Input table type: {in_table.get_data_type(2).str()}")
    print(f"  Output table type: {out_table.get_data_type(1).str()}")
    
    # Note: You'd need to add methods to populate the data in C++
    # For now, this just shows the API structure
    
    print("\nNote: Need to add data population methods to make this fully functional")


if __name__ == "__main__":
    print("=" * 70)
    print("Test: PyArrow with ArrowTable Wrapper")
    print("=" * 70)
    print()
    
    try:
        test_with_pyarrow_direct()
    except TypeError as e:
        print(f"\n✗ TypeError: {e}")
        print("\nThis means the pybind11 bindings need to be updated.")
        print("The exec.run function needs an overload that accepts")
        print("list[ArrowTable] as a parameter.")
    except Exception as e:
        print(f"\n✗ Error: {type(e).__name__}: {e}")
    
    print("\n" + "=" * 70)
    print("Test: Native ArrowTable API Demo")
    print("=" * 70)
    print()
    test_native_arrowtable()