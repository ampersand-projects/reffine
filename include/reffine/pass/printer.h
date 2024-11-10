#ifndef INCLUDE_REFFINE_PASS_PRINTER_H_
#define INCLUDE_REFFINE_PASS_PRINTER_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "llvm/IR/Module.h"
#include "reffine/pass/irpass.h"

using namespace std;

namespace reffine {

class IRPrinterCtx : public IRPassCtx {
public:
    IRPrinterCtx(const shared_ptr<Func> func) :
        IRPassCtx(func->tbl), indent(0), nesting(0)
    {}

private:
    size_t indent;
    size_t nesting;

    friend class IRPrinter;
};

class IRPrinter : public IRPass<IRPrinterCtx> {
public:
    explicit IRPrinter(IRPrinterCtx ctx) : IRPrinter(std::move(ctx), 2) {}

    IRPrinter(IRPrinterCtx ctx, size_t tabstop) :
        _ctx(std::move(ctx)), tabstop(tabstop)
    {}

    static string Build(const shared_ptr<Func> func);
    static string Build(const llvm::Module&);

    void Visit(const SymNode&) final;
    void Visit(const Stmts&) final;
    void Visit(const Func&) final;
    void Visit(const Call&) final;
    void Visit(const IfElse&) final;
    void Visit(const Select&) final;
    void Visit(const Exists&) final;
    void Visit(const Const&) final;
    void Visit(const Cast&) final;
    void Visit(const NaryExpr&) final;
    void Visit(const Read&) final;
    void Visit(const Write&) final;
    void Visit(const Alloc&) final;
    void Visit(const Load&) final;
    void Visit(const Store&) final;
    void Visit(const Loop&) final;
    void Visit(const IsValid&) final;
    void Visit(const SetValid&) final;
    void Visit(const FetchDataPtr&) final;
    void Visit(const NoOp&) final;

private:
    IRPrinterCtx& ctx() { return _ctx; }

    void enter_op() { ctx().nesting++; }
    void exit_op() { ctx().nesting--; }
    void enter_block() { ctx().indent++; emitnewline(); }
    void exit_block() { ctx().indent--; emitnewline(); }

    void emittab() { ostr << string(1 << tabstop, ' '); }
    void emitnewline() { ostr << endl << string(ctx().indent << tabstop, ' '); }
    void emit(string str) { ostr << str; }
    void emitcomment(string comment) { ostr << "/* " << comment << " */"; }

    void emitunary(const string op, const Expr a)
    {
        ostr << op;
        eval(a);
    }

    void emitbinary(const Expr a, const string op, const Expr b)
    {
        ostr << "(";
        eval(a);
        ostr << " " << op << " ";
        eval(b);
        ostr << ")";
    }

    void emitassign(const Expr lhs, const Expr rhs)
    {
        eval(lhs);
        ostr << " = ";
        eval(rhs);
    }

    void emitfunc(const string name, const vector<Expr> args)
    {
        ostr << name << "(";
        for (size_t i = 0; i < args.size(); i++) {
            eval(args[i]);
            ostr << ", ";
        }
        if (args.size() > 0) { ostr << "\b\b"; };
        ostr << ")";
    }

    IRPrinterCtx _ctx;
    size_t tabstop;
    ostringstream ostr;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER_H_
