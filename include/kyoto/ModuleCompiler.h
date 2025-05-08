#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "kyoto/ClassMetadata.h"
#include "kyoto/Resolution/AnalysisVisitor.h"
#include "kyoto/SymbolTable.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

class ASTNode;
class FunctionNode;
class KType;

namespace llvm {
class raw_string_ostream;
class BasicBlock;
class Function;
class StructType;
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

    void add_function(FunctionNode* node);
    std::optional<FunctionNode*> get_function(const std::string& name);

    void register_visitors();
    void register_malloc();

    void push_scope();
    void pop_scope();
    size_t n_scopes() const;

    void set_current_function(FunctionNode* node, llvm::Function* func);
    FunctionNode* get_current_function_node() const;

    void push_class(std::string name);
    void pop_class();
    std::string get_current_class() const;
    bool class_exists(const std::string& name) const;

    void add_class_metadata(const std::string& name, const ClassMetadata& data);
    llvm::StructType* get_llvm_struct(const std::string& name) const;
    ClassMetadata& get_class_metadata(const std::string& name);

    void push_fn_return_type(KType* type);
    void pop_fn_return_type();
    KType* get_fn_return_type() const;

    void insert_dummy_return(llvm::BasicBlock& bb);
    llvm::BasicBlock* create_basic_block(const std::string& name);

private:
    bool verify_module(llvm::raw_string_ostream& os) const;
    ASTNode* parse_program();
    void ensure_main_fn() const;
    void llvm_pass();

private:
    std::unordered_map<std::string, FunctionNode*> functions;
    FunctionNode* current_fn_node = nullptr;
    llvm::Function* current_fn = nullptr;
    KType* curr_fn_ret_type = nullptr;

    std::string code;
    std::string name;

    std::unordered_set<std::string> classes;
    std::string current_class;

    std::vector<std::unique_ptr<IAnalysisVisitor>> analysis_visitors;
    std::unordered_map<std::string, ClassMetadata> classes_metadata;

    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    SymbolTable symbol_table;
    TypeResolver type_resolver {};
};
