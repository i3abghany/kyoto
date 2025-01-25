#include "kyoto/AST/Expressions/UnaryNode.h"

#include <assert.h>
#include <fmt/core.h>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace llvm {
class Value;
}

UnaryNode::UnaryNode(ExpressionNode* expr, UnaryOp op, ModuleCompiler& compiler)
    : type(nullptr)
    , expr(expr)
    , op(op)
    , compiler(compiler)
{
}

UnaryNode::~UnaryNode()
{
    // We only delete the pointer wrapper without deleting the type itself
    if (op == UnaryOp::AddressOf) operator delete(type);
    delete expr;
}

std::string UnaryNode::to_string() const
{
    return fmt::format("Unary({}, {})", op_to_string(), expr->to_string());
}

llvm::Value* UnaryNode::gen()
{
    auto* expr_val = expr->gen();

    if (op == UnaryOp::Negate) return compiler.get_builder().CreateNeg(expr_val, "negval");
    if (op == UnaryOp::Positive) return expr_val;
    if (op == UnaryOp::PrefixDecrement) return gen_prefix_decrement();
    if (op == UnaryOp::PrefixIncrement) return gen_prefix_increment();
    if (op == UnaryOp::AddressOf) {
        auto* ptr = expr->gen_ptr();
        if (!ptr) {
            throw std::runtime_error(fmt::format("Cannot take the address of expression `{}` (type `{}`)",
                                                 expr->to_string(), expr->get_ktype()->to_string()));
        }
        return ptr;
    }

    if (op == UnaryOp::Dereference) {
        auto* expr_ktype = expr->get_ktype();
        if (!expr_ktype->is_pointer()) {
            throw std::runtime_error(
                fmt::format("Cannot dereference non-pointer type `{}`", expr->get_ktype()->to_string()));
        }
        const auto* pointee_type = expr_ktype->as<PointerType>()->get_pointee();
        auto* addr = expr->gen();
        return compiler.get_builder().CreateLoad(get_llvm_type(pointee_type, compiler), addr, "deref");
    }

    assert(false && "Unknown unary operator");
}

llvm::Value* UnaryNode::gen_ptr() const
{
    if (op == UnaryOp::AddressOf) return expr->gen_ptr();
    else if (op == UnaryOp::Dereference) {
        auto* expr_ktype = expr->get_ktype();
        if (!expr_ktype->is_pointer()) {
            throw std::runtime_error(
                fmt::format("Cannot dereference non-pointer type `{}`", expr->get_ktype()->to_string()));
        }
        auto* addr = expr->gen();
        return addr;
    }
    return nullptr;
}

KType* UnaryNode::get_ktype() const
{
    if (op == UnaryOp::AddressOf) {
        if (!type) type = new PointerType(expr->get_ktype());
        return type;
    }
    if (op == UnaryOp::Dereference) {
        auto* expr_ktype = expr->get_ktype();
        if (!expr_ktype->is_pointer()) {
            throw std::runtime_error(
                fmt::format("Cannot dereference non-pointer type `{}`", expr->get_ktype()->to_string()));
        }
        return expr_ktype->as<PointerType>()->get_pointee();
    }
    return expr->get_ktype();
}

llvm::Value* UnaryNode::gen_prefix_increment() const
{
    assert(op == UnaryOp::PrefixIncrement && "Expected prefix increment unary");

    auto* expr_ktype = expr->get_ktype()->as<PrimitiveType>();
    auto* expr_ltype = expr->gen_type();
    if (expr->is_trivially_evaluable() || !expr->gen_ptr() || !expr_ktype->is_integer()) {
        throw std::runtime_error(fmt::format("Cannot apply op `++` to {}expression `{}`",
                                             expr_ktype->is_integer() ? "" : "lvalue", expr->to_string()));
    }

    auto* expr_ptr = expr->gen_ptr();
    auto* expr_val = compiler.get_builder().CreateLoad(expr_ltype, expr_ptr, "incptr");
    assert(expr_val && "Expression value must not be null");
    auto* one = llvm::ConstantInt::get(expr_ltype, 1, true);
    auto* new_val = compiler.get_builder().CreateAdd(expr_val, one, "incval");
    compiler.get_builder().CreateStore(new_val, expr_ptr);
    return new_val;
}

llvm::Value* UnaryNode::gen_prefix_decrement() const
{
    assert(op == UnaryOp::PrefixDecrement && "Expected prefix decrement unary");

    auto* expr_ktype = expr->get_ktype()->as<PrimitiveType>();
    auto* expr_ltype = expr->gen_type();
    if (expr->is_trivially_evaluable() || !expr->gen_ptr() || !expr_ktype->is_integer()) {
        throw std::runtime_error(fmt::format("Cannot apply op `--` to {}expression `{}`",
                                             expr_ktype->is_integer() ? "" : "lvalue", expr->to_string()));
    }

    auto* expr_ptr = expr->gen_ptr();
    auto* expr_val = expr->gen();
    auto* one = llvm::ConstantInt::get(expr_ltype, 1, true);
    auto* new_val = compiler.get_builder().CreateSub(expr_val, one, "decval");
    compiler.get_builder().CreateStore(new_val, expr_ptr);
    return new_val;
}

llvm::Value* UnaryNode::trivial_gen()
{
    assert(is_trivially_evaluable() && "Expression is not trivially evaluable");
    auto* expr_val = expr->trivial_gen();
    auto expr_ltype = expr->gen_type();

    if (op == UnaryOp::Negate) {
        const auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(expr_val);
        assert(constant_int && "Trivial value must be a constant int");
        return llvm::ConstantInt::get(expr_ltype, -constant_int->getSExtValue(), true);
    }
    if (op == UnaryOp::Positive) return expr_val;
    assert(false && "Unknown unary operator");
}

bool UnaryNode::is_trivially_evaluable() const
{
    return simple_op() && expr->is_trivially_evaluable();
}

llvm::Type* UnaryNode::gen_type() const
{
    if (op == UnaryOp::AddressOf) return llvm::PointerType::get(compiler.get_context(), 0);
    if (op == UnaryOp::Dereference)
        return expr->get_ktype()->is_pointer() ? llvm::PointerType::get(compiler.get_context(), 0) : expr->gen_type();
    return expr->gen_type();
}

std::string UnaryNode::op_to_string() const
{
    switch (op) {
    case UnaryOp::Negate:
        return "-";
    case UnaryOp::Positive:
        return "+";
    case UnaryOp::PrefixIncrement:
        return "++";
    case UnaryOp::PrefixDecrement:
        return "--";
    case UnaryOp::AddressOf:
        return "&";
    case UnaryOp::Dereference:
        return "*";
    default:
        return "";
    }
}

bool UnaryNode::simple_op() const
{
    return op == UnaryOp::Negate || op == UnaryOp::Positive;
}
