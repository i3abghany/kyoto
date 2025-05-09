#include <cstddef>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "kyoto/KType.h"
#include "kyoto/SymbolTable.h"

namespace llvm {
class AllocaInst;
}

void Scope::add_symbol(const std::string& name, const Symbol& value)
{
    symbols.insert_or_assign(name, value);
}

std::optional<Symbol> Scope::get_symbol(const std::string& name)
{
    return symbols.contains(name) ? std::optional { symbols[name] } : std::nullopt;
}

SymbolTable::SymbolTable() { }

void SymbolTable::push_scope()
{
    scopes.push_back(Scope {});
}

void SymbolTable::pop_scope()
{
    scopes.pop_back();
}

void SymbolTable::add_symbol(const std::string& name, Symbol symbol)
{
    scopes.back().add_symbol(name, std::move(symbol));
}

std::optional<Symbol> SymbolTable::get_symbol(const std::string& name)
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

Symbol Symbol::primitive(llvm::AllocaInst* value, PrimitiveType::Kind kind)
{
    return Symbol(value, new PrimitiveType(kind));
}

Symbol::Symbol(llvm::AllocaInst* value, KType* type)
    : alloc(value)
    , type(type)
{
}