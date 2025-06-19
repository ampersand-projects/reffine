#include "reffine/pass/symanalysis.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

void SymAnalysis::Visit(SymNode& symbol)
{
    auto tmp = this->tmp_sym(symbol);
    if (_syminfo_map[tmp].count == 0) { _ordered_syms.push_back(tmp); }
    _syminfo_map[tmp].count++;

    IRPass::Visit(symbol);
}

std::pair<std::map<Sym, SymInfo>, std::vector<Sym>> SymAnalysis::Build(
    shared_ptr<Func> func)
{
    IRPassCtx ctx(func->tbl);
    SymAnalysis pass(ctx);
    func->Accept(pass);

    return {pass._syminfo_map, pass._ordered_syms};
}
