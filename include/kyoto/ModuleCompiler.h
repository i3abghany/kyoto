#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "kyoto/SymbolTable.h"

namespace llvm {
class AllocaInst;
}

class ModuleCompiler {
public:
    explicit ModuleCompiler(const std::string& code, const std::string& name = "main");

    void compile();

    llvm::LLVMContext& get_context() { return context; }

    llvm::IRBuilder<>& get_builder() { return builder; }

    llvm::Module* get_module() { return module.get(); }

    std::optional<llvm::AllocaInst*> get_symbol(const std::string& name);
    void add_symbol(const std::string& name, llvm::AllocaInst* value);

    void push_scope();
    void pop_scope();

    size_t n_scopes() const;

private:
    void verify_module() const;

private:
    std::string code;
    std::string name;
    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    SymbolTable symbol_table;
};