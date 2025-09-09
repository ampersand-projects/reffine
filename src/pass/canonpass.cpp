#include "reffine/pass/canonpass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

void CanonPass::Visit(Loop& loop)
{
    IRPass::Visit(loop);

    if (loop.incr) {
        loop.body = _stmts(vector<Stmt>{loop.body, loop.incr});

        loop.incr = nullptr;
    }

    if (loop.body_cond) {
        loop.body = _ifelse(loop.body_cond, loop.body, _noop());

        loop.body_cond = nullptr;
    }
}

void CanonPass::Visit(Func& func)
{
    IRPass::Visit(func);

    if (!func.output->type.is_void()) {
        auto output_addr = _sym("output_addr", func.output->type.ptr());
        func.inputs.insert(func.inputs.begin(), output_addr);

        func.output = _stmtexpr(_store(output_addr, func.output));
    }
}

void CanonPass::Build(shared_ptr<Func> func)
{
    CanonPass pass(make_unique<IRPassCtx>(func->tbl));
    func->Accept(pass);
}
