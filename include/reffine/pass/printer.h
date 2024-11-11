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

    static string Build(Stmt);
    static string Build(llvm::Module&);

    void Visit(SymNode&) override;
    void Visit(Stmts&) override;
    void Visit(Func&) override;
    void Visit(Call&) override;
    void Visit(IfElse&) override;
    void Visit(Select&) override;
    void Visit(Exists&) override;
    void Visit(Const&) override;
    void Visit(Cast&) override;
    void Visit(NaryExpr&) override;
    void Visit(Read&) override;
    void Visit(Write&) override;
    void Visit(Alloc&) override;
    void Visit(Load&) override;
    void Visit(Store&) override;
    void Visit(Loop&) override;
    void Visit(IsValid&) override;
    void Visit(SetValid&) override;
    void Visit(FetchDataPtr&) override;
    void Visit(NoOp&) override;

private:
    void enter_op() { ctx.nesting++; }
    void exit_op() { ctx.nesting--; }
    void enter_block() { ctx.indent++; emitnewline(); }
    void exit_block() { ctx.indent--; emitnewline(); }

    void emittab() { ostr << string(1 << tabstop, ' '); }
    void emitnewline() { ostr << endl << string(ctx.indent << tabstop, ' '); }
    void emit(string str) { ostr << str; }
    void emitcomment(string comment) { ostr << "/* " << comment << " */"; }

    void emitunary(string op, Expr a)
    {
        ostr << op;
        a->Accept(*this);
    }

    void emitbinary(Expr a, string op, Expr b)
    {
        ostr << "(";
        a->Accept(*this);
        ostr << " " << op << " ";
        b->Accept(*this);
        ostr << ")";
    }

    void emitassign(Expr lhs, Expr rhs)
    {
        lhs->Accept(*this);
        ostr << " = ";
        rhs->Accept(*this);
    }

    void emitfunc(string name, vector<Expr> args)
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
