import ir

in_sym_1 = ir._sym("x", ir.DataType(ir.BaseType.i32))
in_sym_2 = ir._sym("y", ir.DataType(ir.BaseType.i32))

temp1 = ir._mul(in_sym_1, ir._const(ir.DataType(ir.BaseType.i32), 2))
temp2 = ir._sqrt(in_sym_1)
cond = ir._gte(temp1, temp2)
res = ir._select(cond, temp1, temp2)

func = ir._func("foo", res, [in_sym_1, in_sym_2], {})

print(f"Example Function:\n{ir.to_string(func)}")
