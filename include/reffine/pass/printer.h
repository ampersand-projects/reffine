#ifndef INCLUDE_REFFINE_PASS_PRINTER_H_
#define INCLUDE_REFFINE_PASS_PRINTER_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "llvm/IR/Module.h"
#include "reffine/pass/base/irgen.h"

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

struct CodeSegBase {
    virtual string to_string(int indent) = 0;
    virtual ~CodeSegBase() {};
};
typedef shared_ptr<CodeSegBase> CodeSeg;

struct StrSeg : public CodeSegBase {
    string str;

    StrSeg(string str) : CodeSegBase(), str(str) {}
    ~StrSeg() override {}

    string to_string(int indent = 0) override
    {
        return str;
    }
};

struct LineSeg : public StrSeg {
    LineSeg(string line) : StrSeg(line) {}
    ~LineSeg() override {}

    string to_string(int indent) override
    {
        return '\n' + string(indent, '\t') + StrSeg::to_string();
    }
};

struct BlockSeg : public CodeSegBase {
    vector<CodeSeg> lines;

    ~BlockSeg() override {}

    string to_string(int indent) override
    {
        stringstream sstr;
        for (const auto& cl : lines) {
            sstr << cl->to_string(indent + 1);
        }
        return sstr.str();
    }
};

class IRPrinter2Ctx : public ValGenCtx<CodeSeg> {
public:
    IRPrinter2Ctx() : ValGenCtx<CodeSeg>() {}
    IRPrinter2Ctx(const SymTable tbl) : ValGenCtx<CodeSeg>(tbl) {}

private:
    shared_ptr<BlockSeg> block = make_shared<BlockSeg>();

    friend class IRPrinter2;
};

class IRPrinter2 : public ValGen<CodeSeg> {
public:
    explicit IRPrinter2(IRPrinter2Ctx ctx) : ValGen<CodeSeg>(ctx), _ctx(ctx) {}

    static string Build(Stmt);

private:
    void Visit(SymNode& symbol) final
    {
        IRGenBase<CodeSeg>::Visit(symbol);
    }

    CodeSeg visit(Sym);
    CodeSeg visit(StmtExprNode& expr);
    CodeSeg visit(Select&);
    CodeSeg visit(IfElse&);
    CodeSeg visit(Const&);
    CodeSeg visit(Cast&);
    CodeSeg visit(Get&);
    CodeSeg visit(New&);
    CodeSeg visit(NaryExpr&);
    CodeSeg visit(Op&);
    CodeSeg visit(Element&);
    CodeSeg visit(Lookup&);
    CodeSeg visit(Locate&);
    CodeSeg visit(NotNull&);
    CodeSeg visit(Reduce&);
    CodeSeg visit(Call&);
    CodeSeg visit(Stmts&);
    CodeSeg visit(Alloc&);
    CodeSeg visit(Load&);
    CodeSeg visit(Store&);
    CodeSeg visit(ThreadIdx&);
    CodeSeg visit(BlockIdx&);
    CodeSeg visit(BlockDim&);
    CodeSeg visit(GridDim&);
    CodeSeg visit(Loop&);
    CodeSeg visit(IsValid&);
    CodeSeg visit(SetValid&);
    CodeSeg visit(FetchDataPtr&);
    CodeSeg visit(NoOp&);
    void visit(Func&);

    void emit(string str)
    {
        this->_ctx.block->lines.push_back(make_shared<StrSeg>(str));
    }

    void emit(CodeSeg seg)
    {
        this->_ctx.block->lines.push_back(seg);
    }

    void emitline(string str = "")
    {
        this->_ctx.block->lines.push_back(make_shared<LineSeg>(str));
    }

    shared_ptr<BlockSeg> enter_block()
    {
        auto old_block = this->_ctx.block;
        auto new_block = make_shared<BlockSeg>();

        old_block->lines.push_back(new_block);
        std::swap(this->_ctx.block, new_block);

        return old_block;
    }

    void exit_block(shared_ptr<BlockSeg> old_block)
    {
        std::swap(old_block, this->_ctx.block);
    }

    IRPrinter2Ctx _ctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER_H_
