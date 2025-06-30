#include "kyoto/AST/Expressions/MethodCallNode.h"

#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"
#include "kyoto/AST/Expressions/IdentifierNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"

MethodCall::MethodCall(std::string instance_name, std::string name, std::vector<ExpressionNode*> args,
                       ModuleCompiler& compiler)
    : FunctionCall(std::move(name), std::move(args), compiler)
    , instance_name(std::move(instance_name))
{
}

MethodCall::~MethodCall() = default;

std::string MethodCall::to_string() const
{
    return instance_name + "." + FunctionCall::to_string();
}

llvm::Value* MethodCall::gen()
{
    auto instance_symbol = compiler.get_symbol(instance_name);
    if (!instance_symbol.has_value()) {
        throw std::runtime_error(std::format("MethodCall::gen: Symbol {} not found", instance_name));
    }

    ExpressionNode* self = new IdentifierExpressionNode(instance_name, compiler);
    if (!instance_symbol.value().type->is_pointer_to_class("")) {
        self = new UnaryNode(self, UnaryNode::UnaryOp::AddressOf, compiler);
    }

    insert_arg(self, 0);
    auto class_name = self->get_ktype()->get_class_name();
    set_name_prefix(class_name + "_");
    return FunctionCall::gen();
}

llvm::Value* MethodCall::gen_ptr() const
{
    auto* ident = new IdentifierExpressionNode(instance_name, compiler);
    auto* ptr = new UnaryNode(ident, UnaryNode::UnaryOp::AddressOf, compiler);
    const_cast<MethodCall*>(this)->insert_arg(ident, 0);
    auto class_name = ident->get_ktype()->get_class_name();
    const_cast<MethodCall*>(this)->set_name_prefix(class_name + "_");
    return FunctionCall::gen_ptr();
}

llvm::Type* MethodCall::gen_type() const
{
    initalize_name_prefix();
    return FunctionCall::gen_type();
}

KType* MethodCall::get_ktype() const
{
    initalize_name_prefix();
    return FunctionCall::get_ktype();
}

llvm::Value* MethodCall::trivial_gen()
{
    initalize_name_prefix();
    return FunctionCall::trivial_gen();
}

bool MethodCall::is_trivially_evaluable() const
{
    initalize_name_prefix();
    return FunctionCall::is_trivially_evaluable();
}

void MethodCall::initalize_name_prefix() const
{
    auto dummy = std::make_unique<IdentifierExpressionNode>(instance_name, compiler);
    auto class_name = dummy->get_ktype()->get_class_name();
    const_cast<MethodCall*>(this)->set_name_prefix(class_name + "_");
}