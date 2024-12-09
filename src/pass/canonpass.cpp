#include "reffine/pass/canonpass.h"

using namespace reffine;

void CanonPass::Visit(Loop& loop)
{
    IRPass::Visit(loop);

    if (loop.incr) {
        loop.body = make_shared<Stmts>(vector<Stmt>{loop.body, loop.incr});

        loop.incr = nullptr;
    }
}

void CanonPass::Build(shared_ptr<Func> func)
{
    CanonPass pass(func);
    func->Accept(pass);
}
