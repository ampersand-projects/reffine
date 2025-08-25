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
        return '\n' + string(indent, '\t');
    }
};

struct BlockSeg : public CodeSegBase {
    vector<CodeSeg> segs;

    string to_string(size_t indent) final
    {
        stringstream sstr;
        for (auto seg : segs) {
            sstr << seg->to_string(indent + 1);
        }
        return sstr.str();
    }
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
    IRPrinter2(IRPrinter2Ctx& ctx) : IRGenBase<CodeSeg>(ctx), _block(make_shared<BlockSeg>()) {}

    static string Build(shared_ptr<Func>);

    CodeSeg visit(Sym) final;
    CodeSeg visit(StmtExprNode&) final;
    CodeSeg visit(New&) final;
    CodeSeg visit(Op&) final;
    CodeSeg visit(Reduce&) final;
    CodeSeg visit(Element&) final;
    CodeSeg visit(MakeVector&) final;
    CodeSeg visit(NotNull&) final;
    CodeSeg visit(Call&) final;
    CodeSeg visit(Lookup&) final;
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
    void visit(Func&) final;

private:
    void emit() {}

    template<typename... R>
    void emit(string str, R... r) {
        this->emit(make_shared<StrSeg>(str), r...);
    }

    template<typename... R>
    void emit(CodeSeg seg, R... r) {
        this->_block->segs.push_back(seg);
        this->emit(r...);
    }

    CodeSeg _str(string str)
    {
        return make_shared<StrSeg>(str);
    }

    CodeSeg _nl()
    {
        return make_shared<NewLineSeg>();
    }

    shared_ptr<BlockSeg> _blk() { return make_shared<BlockSeg>(); }

    shared_ptr<BlockSeg> enter_block(shared_ptr<BlockSeg> child)
    {
        auto parent = this->_block;
        this->_block = child;
        this->emit(this->_nl());
        return parent;
    }

    void exit_block(shared_ptr<BlockSeg> parent)
    {
        this->_block = parent;
    }

    shared_ptr<BlockSeg> _block;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_PRINTER2_H_
