#ifndef INCLUDE_REFFINE_PASS_SCALARPASS_H_
#define INCLUDE_REFFINE_PASS_SCALARPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

class LoadStoreExpand : public IRClone {
private:
    Expr visit(Load&) final;
    Expr visit(Store&) final;
};

class NewGetElimination : public IRClone {
private:
    Expr visit(Select&) final;
    Expr visit(New&) final;
    Expr visit(Get&) final;
    Expr visit(Func&) final;

    map<Expr, vector<Expr>> _new_get_map;
};

class ScalarPass : public IRClone {
private:
    Expr visit(Alloc&) final;
    Expr visit(Load&) final;
    Expr visit(Store&) final;
    Expr visit(Select&) final;
    Expr visit(New&) final;
    Expr visit(Get&) final;
    Expr visit(Func&) final;
    void assign(Sym, Expr) final;

    map<Expr, vector<Expr>>& scalar() { return this->_scalar_map; }

    map<Expr, vector<Expr>> _scalar_map;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SCALARPASS_H_
