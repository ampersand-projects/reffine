#ifndef INCLUDE_REFFINE_PASS_PRINTER_H_
#define INCLUDE_REFFINE_PASS_PRINTER_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "llvm/IR/Module.h"
#include "reffine/pass/visitor.h"

using namespace std;

namespace reffine {

class IRPrinterCtx {
public:
    IRPrinterCtx() : indent(0), nesting(0) {}

private:
    size_t indent;
    size_t nesting;

    friend class IRPrinter;
};

class IRPrinter : public Visitor {
public:
    IRPrinter() : IRPrinter(IRPrinterCtx()) {}
    explicit IRPrinter(IRPrinterCtx ctx) : IRPrinter(std::move(ctx), 2) {}

    IRPrinter(IRPrinterCtx ctx, size_t tabstop) :
        ctx(std::move(ctx)), tabstop(tabstop)
    {}

    static string Build(const Stmt);
    static string Build(const llvm::Module&);

    void Visit(const SymNode&) override;
    void Visit(const Stmts&) override;
    void Visit(const Func&) override;
    void Visit(const Call&) override;
    void Visit(const IfElse&) override;
    void Visit(const Select&) override;
    void Visit(const Exists&) override;
    void Visit(const Const&) override;
    void Visit(const Cast&) override;
    void Visit(const NaryExpr&) override;
    void Visit(const Read&) override;
    void Visit(const Write&) override;
    void Visit(const Alloc&) override;
    void Visit(const Load&) override;
    void Visit(const Store&) override;
    void Visit(const Loop&) override;
    void Visit(const IsValid&) override;
    void Visit(const SetValid&) override;
    void Visit(const FetchDataPtr&) override;
    void Visit(const NoOp&) override;

private:
    void enter_op() { ctx.nesting++; }
    void exit_op() { ctx.nesting--; }
    void enter_block() { ctx.indent++; emitnewline(); }
    void exit_block() { ctx.indent--; emitnewline(); }

    void emittab() { ostr << string(1 << tabstop, ' '); }
    void emitnewline() { ostr << endl << string(ctx.indent << tabstop, ' '); }
    void emit(string str) { ostr << str; }
    void emitcomment(string comment) { ostr << "/* " << comment << " */"; }

    void emitunary(const string op, const Expr a)
    {
        ostr << op;
        a->Accept(*this);
    }

    void emitbinary(const Expr a, const string op, const Expr b)
    {
        ostr << "(";
        a->Accept(*this);
        ostr << " " << op << " ";
        b->Accept(*this);
        ostr << ")";
    }

    void emitassign(const Expr lhs, const Expr rhs)
    {
        lhs->Accept(*this);
        ostr << " = ";
        rhs->Accept(*this);
    }

    void emitfunc(const string name, const vector<Expr> args)
    {
        ostr << name << "(";
        for (size_t i = 0; i < args.size(); i++) {
            args[i]->Accept(*this);
            ostr << ", ";
        }
        if (args.size() > 0) { ostr << "\b\b"; };
        ostr << ")";
    }

    IRPrinterCtx ctx;
    size_t tabstop;
    ostringstream ostr;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER_H_
