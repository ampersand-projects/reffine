#ifndef INCLUDE_REFFINE_PASS_SYMANALYSIS_H_
#define INCLUDE_REFFINE_PASS_SYMANALYSIS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/base/irpass.h"

namespace reffine {

struct SymInfo {
    int count = 0;
};

class SymAnalysis : public IRPass {
public:
    explicit SymAnalysis(IRPassCtx& ctx) : IRPass(ctx) {}

    static map<Sym, SymInfo> Build(shared_ptr<Func>);

private:
    void Visit(SymNode&) final;

    map<Sym, SymInfo> _syminfo_map;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SYMANALYSIS_H_
