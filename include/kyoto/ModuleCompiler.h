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
class BasicBlock;
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

    void push_fn_return_type(KType* type);
    void pop_fn_return_type();
    KType* get_fn_return_type() const;

    void set_fn_termination_error();
    void insert_dummy_return(llvm::BasicBlock& bb);

    size_t n_scopes() const;

private:
    bool verify_module(llvm::raw_string_ostream& os) const;
    ASTNode* parse_program();
    void ensure_main_fn();
    void llvm_pass();

private:
    KType* curr_fn_ret_type = nullptr;
    bool fn_termination_error = false;

    std::string code;
    std::string name;

    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    SymbolTable symbol_table;
    TypeResolver type_resolver {};
};
