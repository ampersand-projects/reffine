#include "reffine/pass/loopgen.h"

using namespace std;
using namespace reffine;

void LoopGen::visit(Func& func)
{
    // Populate loop function inputs
    for (const auto& op_input : func.inputs) {
        auto loop_input = make_shared<SymNode>(op_input->name, op_input);
        _ctx.loop_func->inputs.push_back(loop_input);
        map_sym(op_input, loop_input);
    }

    auto loop_output = eval(func.output);

    // Set loop function output
    auto loop_output_sym = make_shared<SymNode>("loop_output", loop_output);
    map_val(loop_output_sym, loop_output);
    _ctx.loop_func->output = loop_output;
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = make_shared<Func>(op_func->name + "_loop", nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
