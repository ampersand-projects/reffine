#ifndef INCLUDE_REFFINE_PASS_READWRITEPASS_H_
#define INCLUDE_REFFINE_PASS_READWRITEPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

class ReadWritePass : public IRClone {
private:
    Expr visit(ReadData&) final;
    Expr visit(WriteData&) final;
    Expr visit(ReadBit&) final;
    Expr visit(WriteBit&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_READWRITEPASS_H_
