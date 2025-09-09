#include "reffine/pass/symanalysis.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

void SymAnalysis::Visit(SymNode& symbol)
{
    auto tmp = this->tmp_sym(symbol);

    IRPass::Visit(symbol);
    if (_syminfo_map[tmp].count == 0) {
        _syminfo_map[tmp].order = _cur_order++;
    }
    _syminfo_map[tmp].count++;
}

map<Sym, SymInfo> SymAnalysis::Build(shared_ptr<Func> func)
{
    SymAnalysis pass(make_unique<IRPassCtx>(func->tbl));
    func->Accept(pass);

    return pass._syminfo_map;
}
