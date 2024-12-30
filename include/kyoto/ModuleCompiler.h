#pragma once

#include <any>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "antlr4-runtime.h"

#include "kyoto/AST/ASTNode.h"
#include "kyoto/SymbolTable.h"
#include "kyoto/Visitor.h"

class ModuleCompiler {
public:
    ModuleCompiler(const std::string& name = "main", const std::string& code = "");

    void compile();

    llvm::LLVMContext& get_context() { return context; }

    llvm::IRBuilder<>& get_builder() { return builder; }

    llvm::Module* get_module() { return module.get(); }

    std::optional<llvm::AllocaInst*> get_symbol(const std::string& name) { return symbol_table.get_symbol(name); }
    void add_symbol(const std::string& name, llvm::AllocaInst* value) { symbol_table.add_symbol(name, value); }

    void push_scope() { symbol_table.push_scope(); }
    void pop_scope() { symbol_table.pop_scope(); }

private:
    std::string name;
    std::string code;
    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    SymbolTable symbol_table;
};