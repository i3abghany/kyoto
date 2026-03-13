#include "kyoto/AST/Expressions/MethodCallNode.h"

#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

MethodCall::MethodCall(ExpressionNode* instance, std::string instance_text, std::string name,
                       std::vector<ExpressionNode*> args, ModuleCompiler& compiler)
    : FunctionCall(std::move(name), std::move(args), compiler)
    , instance(instance)
    , instance_text(std::move(instance_text))
{
}

MethodCall::~MethodCall()
{
    if (!prepared) delete instance;
}

std::string MethodCall::to_string() const
{
    return instance_text + "." + FunctionCall::to_string();
}

llvm::Value* MethodCall::gen()
{
    prepare_call();
    return FunctionCall::gen();
}

llvm::Value* MethodCall::gen_ptr() const
{
    prepare_call();
    return FunctionCall::gen_ptr();
}

llvm::Type* MethodCall::gen_type() const
{
    prepare_call();
    return FunctionCall::gen_type();
}

KType* MethodCall::get_ktype() const
{
    prepare_call();
    return FunctionCall::get_ktype();
}

llvm::Value* MethodCall::trivial_gen()
{
    prepare_call();
    return FunctionCall::trivial_gen();
}

bool MethodCall::is_trivially_evaluable() const
{
    prepare_call();
    return FunctionCall::is_trivially_evaluable();
}

std::vector<ASTNode*> MethodCall::get_children() const
{
    auto children = FunctionCall::get_children();
    if (!prepared && instance) {
        children.insert(children.begin(), instance);
    }
    return children;
}

void MethodCall::prepare_call() const
{
    if (prepared) return;
    if (!instance) {
        throw std::runtime_error("MethodCall::prepare_call: missing instance expression");
    }

    auto* instance_type = instance->get_ktype();
    auto class_name = instance_type->get_class_name();
    ExpressionNode* self = instance;
    if (!instance_type->is_pointer_to_class("")) {
        self = new UnaryNode(instance, UnaryNode::UnaryOp::AddressOf, compiler);
    }

    const_cast<MethodCall*>(this)->insert_arg(self, 0);
    const_cast<MethodCall*>(this)->set_name_prefix(class_name + "_");
    const_cast<MethodCall*>(this)->instance = nullptr;
    const_cast<MethodCall*>(this)->prepared = true;
}
