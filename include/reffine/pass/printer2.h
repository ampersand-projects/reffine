#ifndef INCLUDE_REFFINE_PASS_PRINTER2_H_
#define INCLUDE_REFFINE_PASS_PRINTER2_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "reffine/pass/base/irgen.h"

using namespace std;

namespace reffine {

struct CodeSegBase {
    virtual ~CodeSegBase() {}
    virtual string to_string(size_t) = 0;
};
typedef shared_ptr<CodeSegBase> CodeSeg;

struct StrSeg : public CodeSegBase {
    string str;

    StrSeg(string str) : CodeSegBase(), str(str) {}

    string to_string(size_t indent) final { return str; }
};

struct NewLineSeg : public CodeSegBase {
    string to_string(size_t indent) final
    {
        return '\n' + string(4*indent, ' ');
    }
};

struct MultiSeg : public CodeSegBase {
    vector<CodeSeg> segs;
    size_t shift;

    MultiSeg(size_t shift) : CodeSegBase(), shift(shift) {}

    void emit() {}

    template<typename... R>
    void emit(string str, R... r) {
        this->emit(make_shared<StrSeg>(str), r...);
    }

    template<typename... R>
    void emit(CodeSeg seg, R... r) {
        this->segs.push_back(seg);
        this->emit(r...);
    }

    string to_string(size_t indent) final
    {
        stringstream sstr;
        for (auto seg : this->segs) {
            sstr << seg->to_string(indent + this->shift);
        }
        return sstr.str();
    }
};

struct LineSeg : public MultiSeg {
    LineSeg() : MultiSeg(0) {}
};

struct BlockSeg : public MultiSeg {
    BlockSeg() : MultiSeg(1) {}
};

class IRPrinter2Ctx : public IRPassBaseCtx<CodeSeg> {
public:
    IRPrinter2Ctx(shared_ptr<Func> func, map<Sym, CodeSeg> m = {})
        : IRPassBaseCtx<CodeSeg>(func->tbl, m)
    {
    }
};

class IRPrinter2 : public IRGenBase<CodeSeg> {
public:
    IRPrinter2(IRPrinter2Ctx& ctx) : IRGenBase<CodeSeg>(ctx), _code(make_shared<BlockSeg>()) {}

    static string Build(shared_ptr<Func>);

    CodeSeg visit(Sym) final;
    CodeSeg visit(StmtExprNode&) final;
    CodeSeg visit(New&) final;
    CodeSeg visit(Op&) final;
    CodeSeg visit(Reduce&) final;
    CodeSeg visit(Element&) final;
    CodeSeg visit(NotNull&) final;
    CodeSeg visit(Call&) final;
    CodeSeg visit(IfElse&) final;
    CodeSeg visit(NoOp&) final;
    CodeSeg visit(Select&) final;
    CodeSeg visit(Const&) final;
    CodeSeg visit(Get&) final;
    CodeSeg visit(Cast&) final;
    CodeSeg visit(NaryExpr&) final;
    CodeSeg visit(Stmts&) final;
    CodeSeg visit(Alloc&) final;
    CodeSeg visit(Load&) final;
    CodeSeg visit(Store&) final;
    CodeSeg visit(AtomicOp&) final;
    CodeSeg visit(StructGEP&) final;
    CodeSeg visit(ThreadIdx&) final;
    CodeSeg visit(BlockIdx&) final;
    CodeSeg visit(BlockDim&) final;
    CodeSeg visit(GridDim&) final;
    CodeSeg visit(Loop&) final;
    CodeSeg visit(FetchDataPtr&) final;
    CodeSeg visit(Func&) final;

private:
    CodeSeg nl()
    {
        return make_shared<NewLineSeg>();
    }

    template<typename... T>
    void emit(T... t) {
        this->_code->emit(t...);
    }

    template<typename... T>
    shared_ptr<LineSeg> code(T... t)
    {
        auto line = make_shared<LineSeg>();
        line->emit(t...);
        return line;
    }

    shared_ptr<MultiSeg> enter_block()
    {
        auto parent = this->_code;
        auto child = make_shared<BlockSeg>();
        this->_code = child;
        return parent;
    }

    shared_ptr<MultiSeg> exit_block(shared_ptr<MultiSeg> parent)
    {
        auto child = this->_code;
        this->_code = parent;
        return child;
    }

    shared_ptr<LineSeg> code_binary(Expr a, string op, Expr b)
    {
        return code("(", eval(a), " ", op, " ", eval(b), ")");
    }

    template<typename T>
    shared_ptr<LineSeg> code_args(string open, vector<T> args, string close)
    {
        auto line = code(open);
        line->emit(eval(args[0]));
        for(size_t i = 1; i < args.size(); i++) {
            line->emit(", ", eval(args[i]));
        }
        line->emit(close);
        return line;
    }

    shared_ptr<LineSeg> code_func(string fn, vector<Expr> args)
    {
        return code_args(fn + "(", args, ")");
    }

    shared_ptr<MultiSeg> _code;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER2_H_
