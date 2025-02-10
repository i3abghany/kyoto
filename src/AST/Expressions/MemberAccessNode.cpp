#include "kyoto/AST/Expressions/MemberAccessNode.h"

#include <fmt/core.h>
#include <llvm/IR/DerivedTypes.h>
#include <optional>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/ClassMetadata.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

MemberAccessNode::MemberAccessNode(ExpressionNode* lhs, std::string member, ModuleCompiler& compiler)
    : lhs(lhs)
    , member(std::move(member))
    , compiler(compiler)
{
}

MemberAccessNode::~MemberAccessNode()
{
    delete lhs;
}

std::string MemberAccessNode::to_string() const
{
    return lhs->to_string() + "." + member;
}

llvm::Value* MemberAccessNode::gen()
{
    auto* member_ptr = gen_ptr();
    auto* llvm_type = gen_type();
    return compiler.get_builder().CreateLoad(llvm_type, member_ptr, member);
}

llvm::Value* MemberAccessNode::gen_ptr() const
{
    auto* lhs_val = lhs->gen_ptr();
    auto* lhs_type = lhs->get_ktype();

    validate_member_access(lhs_type);

    auto* class_type = get_class_type(lhs_type);
    auto member_index = get_member_index(lhs_type);

    if (lhs_type->is_pointer_to_class()) {
        lhs_val = compiler.get_builder().CreateLoad(lhs->gen_type(), lhs_val);
    }

    return compiler.get_builder().CreateStructGEP(class_type, lhs_val, member_index);
}

llvm::Type* MemberAccessNode::gen_type() const
{
    auto* ktype = get_ktype();
    return ASTNode::get_llvm_type(ktype, compiler);
}

KType* MemberAccessNode::get_ktype() const
{
    auto* lhs_type = lhs->get_ktype();

    validate_member_access(lhs_type);

    std::string class_name = lhs_type->get_class_name();
    auto& class_metadata = compiler.get_class_metadata(class_name);

    return get_member_declaration(class_metadata)->as<DeclarationStatementNode>()->get_ktype();
}

void MemberAccessNode::validate_member_access(KType* lhs_type) const
{
    if (!lhs_type->is_class() && !lhs_type->is_pointer_to_class()) {
        throw std::runtime_error(
            fmt::format("Member access is only allowed on class instances, got {}", lhs->to_string()));
    }
}

llvm::Type* MemberAccessNode::get_class_type(KType* lhs_type) const
{
    std::string class_name = lhs_type->get_class_name();

    if (!compiler.class_exists(class_name)) {
        throw std::runtime_error(fmt::format("Class `{}` does not exist", class_name));
    }

    return compiler.get_class_metadata(class_name).llvm_type;
}

unsigned MemberAccessNode::get_member_index(KType* lhs_type) const
{
    std::string class_name = lhs_type->get_class_name();
    auto& class_metadata = compiler.get_class_metadata(class_name);
    auto idx = class_metadata.member_idx(member);
    if (!idx.has_value()) {
        throw std::runtime_error(fmt::format("Class `{}` does not have a member with name `{}`", class_name, member));
    }
    return idx.value();
}

const ASTNode* MemberAccessNode::get_member_declaration(const ClassMetadata& class_metadata) const
{
    const auto* member_def = class_metadata.node->get_declaration_of(member);

    if (!member_def) {
        throw std::runtime_error(
            fmt::format("Class `{}` does not have a member with name `{}`", class_metadata.node->get_name(), member));
    }

    return member_def;
}

llvm::Value* MemberAccessNode::trivial_gen()
{
    return nullptr;
}

bool MemberAccessNode::is_trivially_evaluable() const
{
    return false;
}
