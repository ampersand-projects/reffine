#ifndef INCLUDE_REFFINE_PASS_SYMANALYSIS_H_
#define INCLUDE_REFFINE_PASS_SYMANALYSIS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/base/irpass.h"

namespace reffine {

struct SymInfo {
    int count = 0;
    int order;
};

class SymAnalysis : public IRPass {
public:
    explicit SymAnalysis(unique_ptr<IRPassCtx> ctx) : IRPass(std::move(ctx)) {}

    static map<Sym, SymInfo> Build(shared_ptr<Func>);

private:
    void Visit(SymNode&) final;

    map<Sym, SymInfo> _syminfo_map;
    int _cur_order = 0;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SYMANALYSIS_H_
