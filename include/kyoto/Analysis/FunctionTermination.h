#pragma once

#include "llvm/IR/PassManager.h"

class ModuleCompiler;

namespace llvm {
class Function;
class BasicBlock;
}

struct FunctionTerminationPass : public llvm::PassInfoMixin<FunctionTerminationPass> {

    FunctionTerminationPass(ModuleCompiler& compiler);

    llvm::PreservedAnalyses run(llvm::Function& func, llvm::FunctionAnalysisManager& FAM);

    bool is_basic_block_unreachable(llvm::BasicBlock& bb);
    void verify_function_termination(llvm::Function& func);

private:
    ModuleCompiler& compiler;
};
