import ir

in_sym_1 = ir.sym("x", ir.DataType(ir.BaseType.i32))
in_sym_2 = ir.sym("y", ir.DataType(ir.BaseType.i32))

temp1 = ir._mul(in_sym_1, ir.const(ir.DataType(ir.BaseType.i32), 2))
temp2 = ir._sqrt(in_sym_1)
cond = ir.gte(temp1, temp2)
res = ir.select(cond, temp1, temp2)

func = ir.func("foo", res, [in_sym_1, in_sym_2], {})

print(f"Example Function:\n{ir.to_string(func)}")
