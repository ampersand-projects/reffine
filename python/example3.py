import pyreffine.ir as ir
import pyreffine.exec as exec
import numpy as np
import pyarrow as pa
import ctypes

def create_arrow_table():
    """Create an ArrowTable2 with some sample data"""
    # Create a table with 1 dimension, 100 rows, 2 columns (iter column + data column)
    tbl = ir.ArrowTable2("my_table", 1, 100, ["t", "value"], [ir._i64_t, ir._i64_t])
    return tbl

def transform_op(tbl):
    """
    Python equivalent of the C++ transform_op function.
    Creates an operation that filters and transforms data from an ArrowTable2.
    """
    # Create symbols
    t_sym = ir._sym("t", ir._i64_t)
    vec_in_sym = ir._sym("vec_in", tbl.get_data_type())
    
    # Element access: vec_in_sym[{t_sym}]
    elem_expr = ir._elem(vec_in_sym, t_sym)
    elem = ir._sym("elem", elem_expr)
    
    # Constant ten
    ten = ir._sym("ten", ir._i64_t)
    
    # Output expression: elem[0] + ten
    # elem[0] accesses the first field of the struct
    out_expr = ir._add(ir._get(elem, 0), ten)
    out = ir._sym("out", out_expr)
    
    # Create the operation with conditions:
    # (t in vec_in_sym) && (t > ten) && (t < 64)
    condition = ir._and(
        ir._and(
            ir._in(t_sym, vec_in_sym),
            ir._gt(t_sym, ten)
        ),
        ir._lt(t_sym, ir._i64(64))
    )
    
    op = ir._op([t_sym], condition, [out])
    op_sym = ir._sym("op", op)
    
    # Create function
    foo_fn = ir._func("foo", op_sym, [vec_in_sym])
    
    # Populate symbol table
    foo_fn.insert_sym(elem, elem_expr)
    foo_fn.insert_sym(out, out_expr)
    foo_fn.insert_sym(op_sym, op)
    foo_fn.insert_sym(ten, ir._i64(10))
    
    return foo_fn


def test_arrow_op():
    """Test the operation-based Arrow function"""
    # Create input arrow array with struct type
    in_arr = pa.StructArray.from_arrays([
        pa.array([x for x in range(100)]),  # iter column (t)
        pa.array([x * 2 for x in range(100)])  # value column
    ], names=['t', 'value'])
    
    print(f"Input array:\n{in_arr}\n")
    
    # Get C array interface
    _, c_in_arr = in_arr.__arrow_c_array__()
    
    # Create ArrowTable2 from the schema
    tbl = ir.ArrowTable2("input", 1, 100, ["t", "value"], [ir._i64_t, ir._i64_t])
    
    # Generate the function
    fn = transform_op(tbl)
    print(f"Op IR:\n{exec.to_string(fn)}\n\n")
    
    # Compile and execute
    query = exec.compile_op(fn)
    print("Executing query...\n")
    
    # Note: You'll need to handle output collection based on your execution model
    # This might return a filtered/transformed array
    result = exec.run(query, [c_in_arr])
    print(f"Result:\n{result}\n")

def test_arrow_op():
    """Test the operation-based Arrow function"""
    
    # Step 1: Create the input data as PyArrow array
    in_arr = pa.StructArray.from_arrays([
        pa.array([x for x in range(100)]),      # iter column (t)
        pa.array([x * 2 for x in range(100)])   # value column
    ], names=['t', 'value'])
    
    print(f"Input array:\n{in_arr}\n")
    
    # Step 2: Create ArrowTable2 with matching schema (for type inference only)
    # This is used ONLY for generating the correct IR with proper types
    tbl = ir.ArrowTable2("input", 1, 100, ["t", "value"], [ir._i64_t, ir._i64_t])
    
    # Step 3: Generate the function IR
    fn = transform_op(tbl)
    print(f"Op IR:\n{exec.to_string(fn)}\n\n")
    
    # Step 4: Compile the function
    query = exec.compile_op(fn)
    print("Executing query...\n")
    
    # Step 5: Prepare the C array interfaces
    _, c_in_arr = in_arr.__arrow_c_array__()
    
    # Step 6: Create output buffer (pointer to ArrowTable*)
    # The compiled function will allocate and return an ArrowTable* here
    output_ptr = ctypes.c_void_p(0)
    output_capsule = ctypes.pythonapi.PyCapsule_New(
        ctypes.addressof(output_ptr),
        None,
        None
    )
    
    # Step 7: Execute with BOTH output buffer and input
    try:
        result = exec.run(query, [output_capsule, c_in_arr])
        print(f"Result:\n{result}\n")
        
        # Step 8: Convert output back to PyArrow (would need additional binding)
        # For now, the output_ptr.value contains the address of the result ArrowTable
        if output_ptr.value:
            print(f"Output ArrowTable* address: 0x{output_ptr.value:x}")
            # You'd need a binding to convert this back to PyArrow
            
    except Exception as e:
        print(f"Error during execution: {e}")
        
def test_arrow_op2():
    """Test the operation-based Arrow function"""
    
    # Step 1: Create the input data as PyArrow array
    in_arr = pa.StructArray.from_arrays([
        pa.array([x for x in range(100)]),      # iter column (t)
        pa.array([x * 2 for x in range(100)])   # value column
    ], names=['t', 'value'])
    
    print(f"Input array:\n{in_arr}\n")
    
    # Step 2: Create ArrowTable2 with matching schema (for type inference only)
    # This is used ONLY for generating the correct IR with proper types
    tbl = ir.ArrowTable2("input", 1, 100, ["t", "value"], [ir._i64_t, ir._i64_t])
    
    # Step 3: Generate the function IR
    fn = transform_op(tbl)
    print(f"Op IR:\n{exec.to_string(fn)}\n\n")
    
    # Step 4: Compile the function
    query = exec.compile_op(fn)
    print("Executing query...\n")
    
    # Step 5: Prepare the C array interfaces
    _, c_in_arr = in_arr.__arrow_c_array__()
    
    # Step 6: Create output buffer (pointer to ArrowTable*)
    # The compiled function will allocate and return an ArrowTable* here
    output_ptr = ctypes.c_void_p(0)
    output_capsule = ctypes.pythonapi.PyCapsule_New(
        ctypes.addressof(output_ptr),
        None,
        None
    )
    
    # Step 7: Execute with BOTH output buffer and input
    try:
        result = exec.run(query, [output_capsule, c_in_arr])
        print(f"Result:\n{result}\n")
        
        # Step 8: Convert output back to PyArrow (would need additional binding)
        # For now, the output_ptr.value contains the address of the result ArrowTable
        if output_ptr.value:
            print(f"Output ArrowTable* address: 0x{output_ptr.value:x}")
            # You'd need a binding to convert this back to PyArrow
            
    except Exception as e:
        print(f"Error during execution: {e}")
        
        
def test_arrow_op3():
    """Test the operation-based Arrow function"""
    
    # Create input data
    in_arr = pa.StructArray.from_arrays([
        pa.array([x for x in range(100)]),      # iter column (t)
        pa.array([x * 2 for x in range(100)])   # value column
    ], names=['t', 'value'])
    
    print(f"Input array:\n{in_arr}\n")
    
    # Create ArrowTable2 for type inference
    tbl = ir.ArrowTable2("input", 1, 100, ["t", "value"], [ir._i64_t, ir._i64_t])
    
    # Generate and compile
    fn = transform_op(tbl)
    print(f"Op IR:\n{exec.to_string(fn)}\n\n")
    
    query = exec.compile_op(fn)
    print("Executing query...\n")
    
    # Get C array interface
    _, c_in_arr = in_arr.__arrow_c_array__()
    
    # Execute using run_op - much simpler!
    result_capsule = exec.run_op(query, c_in_arr)
    print(f"Execution completed!")
    print(f"Result capsule: {result_capsule}")
    
    # The result_capsule now contains the ArrowTable* pointer
    # You'd need additional bindings to convert it back to PyArrow

# Run the test
if __name__ == "__main__":
    test_arrow_op3()