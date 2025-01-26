#include "kyoto/AST/Expressions/MemberAccessNode.h"

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"

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
    auto* lhs_val = lhs->gen();
    auto* lhs_type = lhs->get_ktype();

    if (!lhs_type->is_class() && !lhs_type->is_pointer_to_class()) {
        throw std::runtime_error("Member access is only allowed on class instances");
    }

    std::string class_name = lhs_type->is_class()
        ? lhs_type->as<ClassType>()->get_name()
        : lhs_type->as<PointerType>()->get_pointee()->as<ClassType>()->get_name();

    auto& class_metadata = compiler.get_class_metadata(class_name);
    auto* class_type = class_metadata.llvm_type;

    auto* member_def = class_metadata.node->get_declaration_of(member);

    if (!member_def) {
        throw std::runtime_error("Class does not have a member with name " + member);
    }

    auto* member_ktype = member_def->as<DeclarationStatementNode>()->get_ktype();

    auto* llvm_type = ASTNode::get_llvm_type(member_ktype, compiler);
    auto member_index = class_metadata.member_idx(member);

    auto* member_ptr = compiler.get_builder().CreateStructGEP(class_type, lhs_val, member_index);
    return compiler.get_builder().CreateLoad(llvm_type, member_ptr, member);
}

llvm::Value* MemberAccessNode::gen_ptr() const
{
    auto* lhs_val = lhs->gen();
    auto* lhs_type = lhs->get_ktype();

    if (!lhs_type->is_class() && !lhs_type->is_pointer_to_class()) {
        throw std::runtime_error("Member access is only allowed on class instances");
    }

    std::string class_name = lhs_type->is_class()
        ? lhs_type->as<ClassType>()->get_name()
        : lhs_type->as<PointerType>()->get_pointee()->as<ClassType>()->get_name();

    auto& class_metadata = compiler.get_class_metadata(class_name);
    auto* class_type = class_metadata.llvm_type;

    auto* member_def = class_metadata.node->get_declaration_of(member);

    if (!member_def) {
        throw std::runtime_error("Class does not have a member with name " + member);
    }

    auto* member_ktype = member_def->as<DeclarationStatementNode>()->get_ktype();

    auto* llvm_type = ASTNode::get_llvm_type(member_ktype, compiler);
    auto member_index = class_metadata.member_idx(member);

    auto* member_ptr = compiler.get_builder().CreateStructGEP(class_type, lhs_val, member_index);
    return member_ptr;
}

llvm::Type* MemberAccessNode::gen_type() const
{
    auto* ktype = get_ktype();
    return ASTNode::get_llvm_type(ktype, compiler);
}

KType* MemberAccessNode::get_ktype() const
{
    auto* lhs_type = lhs->get_ktype();

    if (!lhs_type->is_class() && !lhs_type->is_pointer_to_class()) {
        throw std::runtime_error("Member access is only allowed on class instances");
    }

    std::string class_name = lhs_type->is_class()
        ? lhs_type->as<ClassType>()->get_name()
        : lhs_type->as<PointerType>()->get_pointee()->as<ClassType>()->get_name();

    auto& class_metadata = compiler.get_class_metadata(class_name);
    auto* member_def = class_metadata.node->get_declaration_of(member);

    if (!member_def) {
        throw std::runtime_error("Class does not have a member with name " + member);
    }

    return member_def->as<DeclarationStatementNode>()->get_ktype();
}

llvm::Value* MemberAccessNode::trivial_gen()
{
    return nullptr;
}

bool MemberAccessNode::is_trivially_evaluable() const
{
    return false;
}
