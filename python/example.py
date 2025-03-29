import ir

in_sym = ir.sym("in", ir.DataType(ir.BaseType.i32))
res = ir.binary_expr(ir.DataType(ir.BaseType.i32),
                    ir.MathOp.mul,
                    in_sym,
                    ir.const(ir.DataType(ir.BaseType.i32), 2))

func = ir.func("foo", res, [in_sym], {})

ir.print_IR(func)
