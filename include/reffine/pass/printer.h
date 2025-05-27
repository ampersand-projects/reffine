#ifndef INCLUDE_REFFINE_PASS_PRINTER_H_
#define INCLUDE_REFFINE_PASS_PRINTER_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "llvm/IR/Module.h"
#include "reffine/pass/base/visitor.h"

using namespace std;

namespace reffine {

class IRPrinterCtx {
public:
    IRPrinterCtx() : indent(0) {}

private:
    size_t indent;

    friend class IRPrinter;
};

class IRPrinter : public Visitor {
public:
    IRPrinter() : IRPrinter(IRPrinterCtx()) {}
    explicit IRPrinter(IRPrinterCtx ctx) : IRPrinter(std::move(ctx), 2) {}

    IRPrinter(IRPrinterCtx ctx, size_t tabstop)
        : ctx(std::move(ctx)), tabstop(tabstop)
    {
    }

    static string Build(Stmt);
    static string Build(llvm::Module&);

    void Visit(SymNode&) final;
    void Visit(StmtExprNode&) final;
    void Visit(Stmts&) final;
    void Visit(Func&) final;
    void Visit(Call&) final;
    void Visit(IfElse&) final;
    void Visit(Select&) final;
    void Visit(Const&) final;
    void Visit(Cast&) final;
    void Visit(Get&) final;
    void Visit(New&) final;
    void Visit(NaryExpr&) final;
    void Visit(Op&) final;
    void Visit(Element&) final;
    void Visit(NotNull&) final;
    void Visit(Reduce&) final;
    void Visit(Alloc&) final;
    void Visit(Load&) final;
    void Visit(Store&) final;
    void Visit(StructGEP&) final;
    void Visit(ThreadIdx&) final;
    void Visit(BlockIdx&) final;
    void Visit(BlockDim&) final;
    void Visit(GridDim&) final;
    void Visit(Loop&) final;
    void Visit(IsValid&) final;
    void Visit(SetValid&) final;
    void Visit(Lookup&) final;
    void Visit(Locate&) final;
    void Visit(FetchDataPtr&) final;
    void Visit(NoOp&) final;

private:
    void enter_block()
    {
        ctx.indent++;
        emitnewline();
    }
    void exit_block()
    {
        ctx.indent--;
        emitnewline();
    }

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
        if (args.size() > 0) { ostr << "\b\b"; }
        ostr << ")";
    }

    IRPrinterCtx ctx;
    size_t tabstop;
    ostringstream ostr;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER_H_
