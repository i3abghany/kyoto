#include "kyoto/Analysis/FunctionTermination.h"

#include <algorithm>
#include <format>
#include <llvm/IR/Analysis.h>
#include <llvm/IR/PassManager.h>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace {

std::string llvm_type_to_string(llvm::Type* type)
{
    std::string result;
    llvm::raw_string_ostream os(result);
    type->print(os);
    os.flush();
    return result;
}

std::unordered_set<llvm::BasicBlock*> collect_reachable_blocks(llvm::Function& func)
{
    std::unordered_set<llvm::BasicBlock*> reachable;
    if (func.empty()) return reachable;

    auto* entry_bb = &func.getEntryBlock();
    std::queue<llvm::BasicBlock*> worklist;

    worklist.push(entry_bb);
    reachable.insert(entry_bb);

    while (!worklist.empty()) {
        auto* bb = worklist.front();
        worklist.pop();

        auto* terminator = bb->getTerminator();
        if (!terminator) continue;

        for (auto* succ : llvm::successors(bb)) {
            if (reachable.insert(succ).second) {
                worklist.push(succ);
            }
        }
    }

    return reachable;
}

std::vector<llvm::BasicBlock*> path_to_first_unterminated_block(llvm::Function& func)
{
    if (func.empty()) return {};

    std::queue<llvm::BasicBlock*> worklist;
    std::unordered_set<llvm::BasicBlock*> visited;
    std::unordered_map<llvm::BasicBlock*, llvm::BasicBlock*> predecessor;

    auto* entry = &func.getEntryBlock();
    worklist.push(entry);
    visited.insert(entry);

    while (!worklist.empty()) {
        auto* bb = worklist.front();
        worklist.pop();

        auto* terminator = bb->getTerminator();
        if (!terminator) {
            std::vector<llvm::BasicBlock*> path;
            for (auto* current = bb; current != nullptr;
                 current = predecessor.contains(current) ? predecessor[current] : nullptr) {
                path.push_back(current);
                if (current == entry) break;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (auto* succ : llvm::successors(bb)) {
            if (!visited.insert(succ).second) continue;
            predecessor[succ] = bb;
            worklist.push(succ);
        }
    }

    return {};
}

}

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
    const auto reachable = collect_reachable_blocks(func);
    std::vector<llvm::BasicBlock*> unreachable_bbs;
    for (auto& bb : func) {
        if (reachable.contains(&bb)) continue;

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

    for (auto* i : to_remove) {
        i->dropAllReferences();
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

    remove_unreachable_blocks(func);

    if (func.getReturnType()->isVoidTy()) {
        ensure_void_return(func);
        return;
    }

    if (const auto path = path_to_first_unterminated_block(func); !path.empty()) {
        throw std::runtime_error(std::format(
            "Missing return in function `{}`: reached the end of a reachable control-flow path without returning `{}`",
            func.getName().str(), llvm_type_to_string(func.getReturnType())));
    }
}
