#include "reffine/builder/reffiner.h"
#include "reffine/pass/canonpass.h"

using namespace reffine;
using namespace reffine::reffiner;

void CanonPass::Visit(Loop& loop)
{
    IRPass::Visit(loop);

    if (loop.incr) {
        loop.body = _stmts(vector<Stmt>{loop.body, loop.incr});

        loop.incr = nullptr;
    }
}

void CanonPass::Build(shared_ptr<Func> func)
{
    IRPassCtx ctx(func->tbl);
    CanonPass pass(ctx);
    func->Accept(pass);
}
