#pragma once

#include "llvm/IR/Analysis.h"
#include "llvm/IR/PassManager.h"

class ModuleCompiler;

namespace llvm {
class Function;
class BasicBlock;
}

struct FunctionTerminationPass : llvm::PassInfoMixin<FunctionTerminationPass> {

    explicit FunctionTerminationPass(ModuleCompiler& compiler);

    llvm::PreservedAnalyses run(llvm::Function& func, llvm::FunctionAnalysisManager& FAM);

    bool is_basic_block_unreachable(llvm::BasicBlock& bb);
    void verify_function_termination(llvm::Function& func);
    void ensure_void_return(llvm::Function& func);

private:
    ModuleCompiler& compiler;
};
