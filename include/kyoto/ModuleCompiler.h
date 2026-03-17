#pragma once

#include <cstddef>
#include <filesystem>
#include <format>
#include <llvm/IR/DataLayout.h>
#include <memory>
#include <optional>
#include <set>
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
    explicit ModuleCompiler(const std::string& code, const std::string& name = "main",
                            std::optional<std::filesystem::path> entry_path = std::nullopt);

    std::optional<std::string> gen_ir();

    llvm::LLVMContext& get_context() { return context; }

    llvm::IRBuilder<>& get_builder() { return builder; }

    llvm::Module* get_module() { return module.get(); }

    const std::string& get_code() const { return code; }
    const std::filesystem::path& get_source_path() const { return current_source_path; }
    const std::string& get_current_module_name() const { return current_module_name; }
    TypeResolver& get_type_resolver() { return type_resolver; }

    std::optional<Symbol> get_symbol(const std::string& name);
    void add_symbol(const std::string& name, Symbol symbol);

    void add_function(FunctionNode* node);
    std::optional<FunctionNode*> get_function(const std::string& name);
    std::optional<FunctionNode*> get_function(const std::string& name, size_t arity);
    std::string qualify_local_name(const std::string& name) const;
    std::string qualify_imported_name(const std::string& module_name, const std::string& name) const;
    std::string resolve_module_reference(const std::string& module_name) const;
    bool is_imported_module_visible(const std::string& module_name) const;

    void register_visitors();
    void register_malloc();
    void register_free();

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
    size_t get_type_size(const std::string& name) const;
    size_t get_primitive_size(const std::string& name) const;

    void push_fn_return_type(KType* type);
    void pop_fn_return_type();
    KType* get_fn_return_type() const;

    void insert_dummy_return(llvm::BasicBlock& bb);
    llvm::BasicBlock* create_basic_block(const std::string& name);

    void register_type_alias(const std::string& alias, KType* type);
    KType* resolve_type_alias(const std::string& alias);
    KType* resolve_type_alias(const std::string& module_name, const std::string& alias);
    bool is_type_alias(const std::string& name) const;

    void push_type_alias_scope();
    void pop_type_alias_scope();

    struct TemplateMetadata {
        std::string param;
        std::string raw_text;
    };
    void register_template(const std::string& name, const std::string& param, const std::string& raw_text)
    {
        template_registry[name] = { param, raw_text };
    }
    void instantiate_template(const std::string& name, const std::string& mangled_name, const std::string& type_str);
    std::vector<ASTNode*>& get_instantiated_nodes() { return instantiated_nodes; }

    void set_building_top_level(bool value) { building_top_level = value; }
    bool is_building_top_level() const { return building_top_level; }

private:
    bool verify_module(llvm::raw_string_ostream& os) const;
    ASTNode* parse_program(const std::string& source);
    void ensure_main_fn() const;
    void llvm_pass();
    void load_modules();
    void load_module_recursive(const std::string& module_name, const std::filesystem::path& path,
                               const std::string* source, std::vector<std::string>& stack);
    std::vector<std::string> parse_imports(const std::string& source, const std::filesystem::path& path) const;
    std::unique_ptr<ASTNode> build_module_ast(const std::string& module_name);
    std::vector<std::string> topo_sort_modules() const;
    std::string mangle_module_name(const std::string& module_name) const;
    std::string make_qualified_name(const std::string& module_name, const std::string& name) const;
    std::string module_name_from_qualified_symbol(const std::string& qualified_name) const;
    void enter_module_context(const std::string& module_name);

    std::string make_function_key(const std::string& name, size_t arity) const;

    struct LoadedModule {
        std::string name;
        std::filesystem::path path;
        std::string code;
        std::vector<std::string> imports;
    };

private:
    std::unordered_map<std::string, FunctionNode*> functions;
    FunctionNode* current_fn_node = nullptr;
    llvm::Function* current_fn = nullptr;
    KType* curr_fn_ret_type = nullptr;

    std::string code;
    std::string name;
    std::optional<std::filesystem::path> entry_path;
    std::filesystem::path current_source_path;
    std::string current_module_name;
    bool building_top_level = false;

    std::unordered_set<std::string> classes;
    std::string current_class;

    std::vector<std::unique_ptr<IAnalysisVisitor>> analysis_visitors;
    std::unordered_map<std::string, ClassMetadata> classes_metadata;

    llvm::LLVMContext context {};
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    llvm::DataLayout data_layout;

    SymbolTable symbol_table;
    TypeResolver type_resolver {};

    std::unordered_map<std::string, TemplateMetadata> template_registry;
    std::vector<ASTNode*> instantiated_nodes;

    std::vector<std::unordered_map<std::string, KType*>> type_alias_scopes;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<KType>>> module_type_aliases;
    std::unordered_map<std::string, LoadedModule> loaded_modules;
    std::unordered_map<std::string, std::set<std::string>> module_imports;
};
