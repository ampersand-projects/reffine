#include "reffine/pass/loopgen.h"

using namespace std;
using namespace reffine;



shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    vector<Sym> inputs;
    auto output = make_shared<SymNode>("loop_output", op_func->output);

    auto loop_func = make_shared<Func>(op_func->name + "_loop", output, inputs);

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
