#include "kyoto/Analysis/FunctionTermination.h"

#include <vector>

#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"

FunctionTerminationPass::FunctionTerminationPass(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

llvm::PreservedAnalyses FunctionTerminationPass::run(llvm::Function& func, llvm::FunctionAnalysisManager& FAM)
{
    verify_function_termination(func);
    return llvm::PreservedAnalyses::all();
}

void remove_unreachable_blocks(llvm::Function& func)
{
    std::vector<llvm::BasicBlock*> unreachable_bbs;
    for (auto& bb : func) {
        if (&bb == &func.getEntryBlock()) continue;
        if (llvm::isPotentiallyReachable(&func.getEntryBlock(), &bb)) continue;

        unreachable_bbs.push_back(&bb);
    }

    for (auto* bb : unreachable_bbs) {
        while (!bb->empty()) {
            auto& i = bb->back();
            i.eraseFromParent();
        }
        bb->eraseFromParent();
    }
}

bool has_multiple_terminators(llvm::BasicBlock& bb)
{
    int count = 0;
    for (auto& i : bb) {
        if (i.isTerminator()) {
            count++;
            if (count > 1) return true;
        }
    }
    return false;
}

void ensure_single_terminator(llvm::BasicBlock& bb)
{
    if (!has_multiple_terminators(bb)) return;

    llvm::Instruction* first_terminator = nullptr;
    std::vector<llvm::Instruction*> to_remove;

    for (auto& i : bb) {
        if (first_terminator) {
            to_remove.push_back(&i);
            continue;
        }

        if (i.isTerminator()) first_terminator = &i;
    }

    for (auto& i : to_remove) {
        i->eraseFromParent();
    }
}

void FunctionTerminationPass::ensure_void_return(llvm::Function& func)
{
    for (auto& bb : func) {
        if (bb.getTerminator()) continue;
        if (bb.getParent()->getReturnType()->isVoidTy()) {
            compiler.insert_dummy_return(bb);
        }
    }
}

void FunctionTerminationPass::verify_function_termination(llvm::Function& func)
{
    remove_unreachable_blocks(func);

    for (auto& bb : func) {
        ensure_single_terminator(bb);
    }

    if (func.getReturnType()->isVoidTy()) {
        ensure_void_return(func);
    }
}