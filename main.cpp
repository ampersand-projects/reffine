#include <iostream>
#include <memory>

#include "reffine/ir/node.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"

using namespace reffine;
using namespace std;

/*
shared_ptr<Func> reffine_fn()
{
    auto input = make_shared<SymNode>("in", types::VECTOR<1>({types::INT32, types::INT32}));
    auto output = make_shared<SymNode>("out", types::VECTOR<1>({types::INT32, types::INT32}));

    // construct the loop
    auto loop = make_shared<Loop>(output);
    auto idx = make_shared<SymNode>("idx", types::IDX);
    auto one = make_shared<Const>(BaseType::IDX, 1);
    loop->idx_inits[idx] = one;
    loop->idx_incrs[idx] = make_shared<Add>(idx, one);

    loop->body_cond = make_shared<Const>(BaseType::BOOL, 1);

    auto ten = make_shared<Const>(BaseType::IDX, 10);
    loop->exit_cond = make_shared<LessThan>(idx, ten);

    auto read = make_shared<Read>(input, idx);
    auto two = make_shared<Const>(BaseType::INT32, 2);
    auto add_two = make_shared<Add>(read, two);
    loop->body = make_shared<PushBack>(output, add_two);

    auto loop_sym = make_shared<SymNode>("loop", loop);
    
    // construct the function
    auto inputs = vector<Sym>{input, output};
    auto loop_fn = make_shared<Func>("query", loop_sym, inputs);
    loop_fn->tbl[loop_sym] = loop;

    return loop_fn;
}
*/

shared_ptr<Func> simple_fn()
{
    auto n_sym = make_shared<SymNode>("n", types::INT32);

    auto idx_alloc = make_shared<Alloc>(types::INT32);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto sum_alloc = make_shared<Alloc>(types::INT32);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);

    auto zero = make_shared<Const>(BaseType::INT32, 0);
    auto one = make_shared<Const>(BaseType::INT32, 1);
    auto two = make_shared<Const>(BaseType::INT32, 2);

    auto loop = make_shared<Loop>(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
        make_shared<Store>(sum_addr, one),
    });
    loop->incr = nullptr;
    loop->exit_cond = make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), n_sym);
    loop->body_cond = nullptr;
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, make_shared<Add>(make_shared<Load>(idx_addr), one)),
        make_shared<Store>(sum_addr, make_shared<Mul>(make_shared<Load>(sum_addr), two)),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{n_sym});
    foo_fn->tbl[loop_sym] = loop;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;

    return foo_fn;
}

shared_ptr<Func> abs_fn()
{
    auto a_sym = make_shared<SymNode>("a", types::INT32);
    auto b_sym = make_shared<SymNode>("b", types::INT32);

    auto cond = make_shared<GreaterThan>(a_sym, b_sym);
    auto true_body = make_shared<Sub>(a_sym, b_sym);
    auto false_body = make_shared<Sub>(b_sym, a_sym);
    auto ifelse = make_shared<IfElse>(cond, true_body, false_body);
    auto ifelse_sym = make_shared<SymNode>("abs", ifelse);

    auto foo_fn = make_shared<Func>("foo", ifelse_sym, vector<Sym>{a_sym, b_sym});
    foo_fn->tbl[ifelse_sym] = ifelse;

    return foo_fn;
}

int main()
{
    auto fn = simple_fn();
    cout << IRPrinter::Build(fn) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    cout << IRPrinter::Build(*llmod) << endl;

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<int (*)(int)>(fn->name);

    cout << "Result: " << query_fn(5) << endl;

    return 0;
}
