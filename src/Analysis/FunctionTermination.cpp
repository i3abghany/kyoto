#include "kyoto/Analysis/FunctionTermination.h"

#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

FunctionTerminationPass::FunctionTerminationPass(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

llvm::PreservedAnalyses FunctionTerminationPass::run(llvm::Function& func, llvm::FunctionAnalysisManager& FAM)
{
    verify_function_termination(func);
    return llvm::PreservedAnalyses::all();
}

bool FunctionTerminationPass::is_basic_block_unreachable(llvm::BasicBlock& bb)
{
    if (&bb == &bb.getParent()->getEntryBlock()) {
        return false;
    }

    for (auto* pred : llvm::predecessors(&bb)) {
        if (auto* term = pred->getTerminator(); !llvm::isa<llvm::ReturnInst>(term)) {
            return false;
        }
    }

    return true;
}

void FunctionTerminationPass::verify_function_termination(llvm::Function& func)
{
    bool has_error = false;
    for (auto& bb : func) {
        auto* term = bb.getTerminator();
        if (term && (llvm::isa<llvm::BranchInst>(term) || llvm::isa<llvm::ReturnInst>(term))) {
            continue;
        }

        if (is_basic_block_unreachable(bb)) {
            compiler.insert_dummy_return(bb);
            continue;
        }
        llvm::errs() << "Function " << func.getName() << " does not terminate properly\n";
        has_error = true;
    }

    if (has_error) {
        compiler.set_fn_termination_error();
    }
}
