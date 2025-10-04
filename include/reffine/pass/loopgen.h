#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

class LoopGen : public IRClone {
private:
    pair<shared_ptr<Loop>, vector<Expr>> build_loop(Op&);
    Expr visit(Op&) final;
    Expr visit(Reduce&) final;
    Expr visit(Element&) final;
    Expr visit(Lookup&) final;

    map<Expr, map<Expr, Expr>> _vec_iter_idx_map;  // vec -> iter -> idx
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
