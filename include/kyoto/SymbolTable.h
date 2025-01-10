#pragma once

#include <map>
#include <memory>
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
    KType* type;

    static Symbol primitive(llvm::AllocaInst* value, PrimitiveType::Kind kind);
    Symbol(llvm::AllocaInst* value, bool is_primitive, KType* type);
    Symbol() = default;
};

class Scope {
    std::map<std::string, Symbol> symbols {};

public:
    void add_symbol(const std::string& name, const Symbol& symbol);
    std::optional<Symbol> get_symbol(const std::string& name);
};

class SymbolTable {
public:
    SymbolTable();

    void push_scope();
    void pop_scope();
    void add_symbol(const std::string& name, Symbol symbol);
    std::optional<Symbol> get_symbol(const std::string& name);
    size_t n_scopes() const;

private:
    std::vector<Scope> scopes;
};