#ifndef INCLUDE_REFFINE_PASS_CANONPASS_H_
#define INCLUDE_REFFINE_PASS_CANONPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

class CanonPass : public IRClone {
private:
    Expr visit(Define&) final;
    Expr visit(Get&) final;
    Expr visit(Loop&) final;
    Expr visit(Func&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CANONPASS_H_
