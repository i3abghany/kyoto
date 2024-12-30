#include <cstddef>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "kyoto/SymbolTable.h"

class ModuleCompiler;
namespace llvm {
class AllocaInst;
}
namespace llvm {
class Module;
}

void Scope::add_symbol(const std::string& name, llvm::AllocaInst* value)
{
    symbols[name] = value;
}

std::optional<llvm::AllocaInst*> Scope::get_symbol(const std::string& name)
{
    return symbols.contains(name) ? std::optional { symbols[name] } : std::nullopt;
}

SymbolTable::SymbolTable(ModuleCompiler& compiler, llvm::Module* module)
    : compiler(compiler)
    , module(module)
{
}

void SymbolTable::push_scope()
{
    scopes.push_back(Scope {});
}

void SymbolTable::pop_scope()
{
    scopes.pop_back();
}

void SymbolTable::add_symbol(const std::string& name, llvm::AllocaInst* value)
{
    scopes.back().add_symbol(name, value);
}

std::optional<llvm::AllocaInst*> SymbolTable::get_symbol(const std::string& name)
{
    for (auto& scope : std::ranges::views::reverse(scopes)) {
        if (auto symbol = scope.get_symbol(name); symbol.has_value()) {
            return symbol.value();
        }
    }
    return std::nullopt;
}

size_t SymbolTable::n_scopes() const
{
    return scopes.size();
}