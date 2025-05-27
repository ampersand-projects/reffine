#include "reffine/pass/symanalysis.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

void SymAnalysis::Visit(SymNode& symbol)
{
    auto tmp = this->tmp_sym(symbol);
    _syminfo_map[tmp].count++;

    IRPass::Visit(symbol);
}

map<Sym, SymInfo> SymAnalysis::Build(shared_ptr<Func> func)
{
    IRPassCtx ctx(func->tbl);
    SymAnalysis pass(ctx);
    func->Accept(pass);

    return pass._syminfo_map;
}
