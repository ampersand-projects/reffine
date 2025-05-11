import ir

in_sym_1 = ir._sym("x", ir._i32_t)
in_sym_2 = ir._sym("y", ir._i32_t)

temp1 = ir._mul(in_sym_1, ir._i32(2))
temp2 = ir._sqrt(in_sym_1)
cond = ir._gte(temp1, temp2)
res = ir._select(cond, temp1, temp2)

func = ir._func("foo", res, [in_sym_1, in_sym_2], {})

print(f"Example Function:\n{ir.to_string(func)}")
