#include "reffine/pass/canonpass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr CanonPass::visit(Define& define)
{
    auto new_sym = _sym(define.sym->name, define.sym);
    this->map_sym(define.sym, new_sym);
    this->assign(new_sym, eval(define.val));
    return new_sym;
}

Expr CanonPass::visit(Loop& loop)
{
    auto new_loop = _loop(eval(loop.output));

    new_loop->init = loop.init ? eval(loop.init) : nullptr;
    new_loop->exit_cond = loop.exit_cond ? eval(loop.exit_cond) : nullptr;
    new_loop->body = loop.body ? eval(loop.body) : nullptr;
    new_loop->post = loop.post ? eval(loop.post) : nullptr;
    if (loop.incr) {
        new_loop->body = _stmts(vector<Expr>{new_loop->body, eval(loop.incr)});
    }
    if (loop.body_cond) {
        new_loop->body = _ifelse(eval(loop.body_cond), new_loop->body, _noop());
    }

    return new_loop;
}

Expr CanonPass::visit(Func& func)
{
    IRClone::visit(func);
    auto new_func = this->ctx().out_func;

    if (!new_func->output->type.is_void()) {
        auto output_addr = _sym("output_addr", new_func->output->type.ptr());
        new_func->inputs.insert(new_func->inputs.begin(), output_addr);

        new_func->output = _store(output_addr, new_func->output);
    }

    return new_func;
}
