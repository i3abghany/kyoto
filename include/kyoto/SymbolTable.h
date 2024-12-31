#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "kyoto/KType.h"

class ModuleCompiler;

namespace llvm {
class Module;
class AllocaInst;
class Value;
}

struct Symbol {
    llvm::AllocaInst* alloc;
    bool is_primitive;
    PrimitiveType::Kind kind;

    static Symbol primitive(llvm::AllocaInst* value, PrimitiveType::Kind kind);
    Symbol(llvm::AllocaInst* value, bool is_primitive, PrimitiveType::Kind kind);
    Symbol() = default;
};

class Scope {
    std::map<std::string, Symbol> symbols {};

public:
    void add_symbol(const std::string& name, const Symbol& symbol);
    std::optional<Symbol> get_symbol(const std::string& name);
};

class SymbolTable {
    std::vector<Scope> scopes;
    ModuleCompiler& compiler;
    llvm::Module* module;

public:
    SymbolTable(ModuleCompiler& compiler, llvm::Module* module);

    void push_scope();
    void pop_scope();
    void add_symbol(const std::string& name, Symbol symbol);
    std::optional<Symbol> get_symbol(const std::string& name);
    size_t n_scopes() const;
};