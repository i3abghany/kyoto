#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

class ModuleCompiler;

namespace llvm {
class Module;
class AllocaInst;
}

class Scope {
    std::map<std::string, llvm::AllocaInst*> symbols {};

public:
    void add_symbol(const std::string& name, llvm::AllocaInst* value);
    std::optional<llvm::AllocaInst*> get_symbol(const std::string& name);
};

class SymbolTable {
    std::vector<Scope> scopes;
    ModuleCompiler& compiler;
    llvm::Module* module;

public:
    SymbolTable(ModuleCompiler& compiler, llvm::Module* module);

    void push_scope();
    void pop_scope();
    void add_symbol(const std::string& name, llvm::AllocaInst* value);
    std::optional<llvm::AllocaInst*> get_symbol(const std::string& name);
};