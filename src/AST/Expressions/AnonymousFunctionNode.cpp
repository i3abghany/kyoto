#include "kyoto/AST/Expressions/AnonymousFunctionNode.h"

#include <format>
#include <stdexcept>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

AnonymousFunctionNode::AnonymousFunctionNode(FunctionNode* function, FunctionType* type, ModuleCompiler& compiler)
    : function(function)
    , type(type)
    , compiler(compiler)
{
}

AnonymousFunctionNode::~AnonymousFunctionNode()
{
    delete type;
}

std::string AnonymousFunctionNode::to_string() const
{
    return std::format("AnonymousFunction({})", function->get_name());
}

llvm::Value* AnonymousFunctionNode::gen()
{
    const auto llvm_name = function->is_external()
        ? function->get_linkage_name()
        : function->get_linkage_name() + "_" + std::to_string(function->get_params().size());
    auto* fn = compiler.get_module()->getFunction(llvm_name);
    if (!fn) {
        throw std::runtime_error(std::format("Anonymous function `{}` not found in module", function->get_name()));
    }
    return fn;
}

llvm::Type* AnonymousFunctionNode::gen_type() const
{
    return ASTNode::get_llvm_type(type, compiler);
}

KType* AnonymousFunctionNode::get_ktype() const
{
    return type;
}

std::vector<ASTNode*> AnonymousFunctionNode::get_children() const
{
    return {};
}
