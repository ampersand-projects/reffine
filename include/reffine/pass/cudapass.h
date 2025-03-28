#ifndef INCLUDE_REFFINE_PASS_CUDAGEN_H_
#define INCLUDE_REFFINE_PASS_CUDAGEN_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/base/irpass.h"
#include "llvm/IR/Module.h"

namespace reffine {

class CUDAPass : public IRPass {
public:
    explicit CUDAPass(IRPassCtx& ctx) : IRPass(ctx) {}

    static void Build(shared_ptr<Kernel>, llvm::Module&);

protected:
    // void Visit(Kernel&) final;
    // void Visit(Func&) final;
    void Visit(Loop&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CUDAGEN_H_
