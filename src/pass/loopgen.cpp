#include "reffine/pass/loopgen.h"

using namespace std;
using namespace reffine;

Expr LoopGen::visit(Reduce& red)
{
    return IRClone::visit(red);
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = make_shared<Func>(op_func->name, nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
