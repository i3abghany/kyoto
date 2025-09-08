#include "kyoto/AST/TypeAliasNode.h"

#include <memory>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

TypeAliasNode::TypeAliasNode(KType* originalType, std::string alias, ModuleCompiler& compiler)
    : originalType(originalType)
    , alias(std::move(alias))
    , compiler(compiler)
{
    // Register the type alias immediately during AST construction
    // This allows it to be available for resolution during the same parse phase
    compiler.register_type_alias(this->alias, originalType);
}

std::string TypeAliasNode::to_string() const
{
    return "TypeAlias(" + alias + " -> " + originalType->to_string() + ")";
}

llvm::Value* TypeAliasNode::gen()
{
    // Type aliases don't generate any LLVM IR
    // The alias was already registered during construction
    return nullptr;
}
