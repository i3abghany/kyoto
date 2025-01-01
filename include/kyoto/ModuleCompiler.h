#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "kyoto/SymbolTable.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

class ASTNode;
class KType;
namespace llvm {
class raw_string_ostream;
}

class ModuleCompiler {
public:
    explicit ModuleCompiler(const std::string& code, const std::string& name = "main");

    std::optional<std::string> gen_ir();

    llvm::LLVMContext& get_context() { return context; }

    llvm::IRBuilder<>& get_builder() { return builder; }

    llvm::Module* get_module() { return module.get(); }

    TypeResolver& get_type_resolver() { return type_resolver; }

    std::optional<Symbol> get_symbol(const std::string& name);
    void add_symbol(const std::string& name, Symbol symbol);

    void push_scope();
    void pop_scope();

    size_t n_scopes() const;

private:
    bool verify_module() const;
    ASTNode* parse_program();

private:
    std::string code;
    std::string name;
    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    SymbolTable symbol_table;
    TypeResolver type_resolver {};
};