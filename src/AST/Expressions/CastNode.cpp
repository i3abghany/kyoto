#include "kyoto/AST/Expressions/CastNode.h"

#include <format>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

namespace llvm {
class Value;
}

CastNode::CastNode(KType* type, ExpressionNode* expr, ModuleCompiler& compiler)
    : type(type)
    , expr(expr)
    , compiler(compiler)
{
}

CastNode::~CastNode()
{
    delete type;
    delete expr;
}

std::string CastNode::to_string() const
{
    return std::format("Cast({}, {})", type->to_string(), expr->to_string());
}

void CastNode::throw_incompatible_cast_error(const KType* expr_ktype, const KType* target_ktype) const
{
    throw std::runtime_error(
        std::format("Incompatible cast: cannot cast {} to {}", expr_ktype->to_string(), target_ktype->to_string()));
}

void CastNode::check_compatible_integer_cast(const PrimitiveType* expr_ktype, const PrimitiveType* target_type)
{
    if (expr_ktype->is_boolean() != target_type->is_boolean()) {
        throw_incompatible_cast_error(expr_ktype, target_type);
    }

    if (expr_ktype->width() > target_type->width()) {
        throw_incompatible_cast_error(expr_ktype, target_type);
    }
}

llvm::Value* CastNode::handle_integer_cast()
{
    auto expr_ktype = expr->get_ktype()->as<PrimitiveType>();
    auto target_ktype = type->as<PrimitiveType>();

    if (expr->is_trivially_evaluable()) {
        if (expr_ktype->is_boolean() != target_ktype->is_boolean()) {
            throw_incompatible_cast_error(expr_ktype, target_ktype);
        }

        auto* expr_val = expr->trivial_gen();
        const auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(expr_val);
        if (!constant_int) {
            throw std::runtime_error(std::format("Expression `{}` is not a constant integer", expr->to_string()));
        }

        const auto int_val = target_ktype->is_boolean() || expr_ktype->is_boolean() ? constant_int->getZExtValue()
                                                                                    : constant_int->getSExtValue();
        ExpressionNode::check_int_range_fit(int_val, target_ktype, compiler, expr->to_string(), "");
        return llvm::ConstantInt::get(get_llvm_type(target_ktype, compiler), int_val, true);
    }

    check_compatible_integer_cast(expr_ktype, target_ktype);

    auto* expr_val = expr->gen();
    return ExpressionNode::dynamic_integer_conversion(expr_val, expr_ktype, target_ktype, compiler);
}

llvm::Value* CastNode::gen()
{
    // Supported casting scenarios:
    // - Widening integer conversions (e.g., i8 to i32)
    // - Boolean to boolean casts
    // - Arbitrary pointer conversions (e.g., T* to U*)
    // - Identity casts (e.g., T to T)
    // Prohibited casting scenarios:
    // - Narrowing integer conversions (e.g., i32 to i8)
    // - Pointer to integer or integer to pointer conversions
    // - From/to array types
    // - from/to integer/boolean/char one another

    auto expr_ktype = expr->get_ktype();
    auto target_ktype = type;

    if (expr_ktype->is_integer() && target_ktype->is_integer()) {
        return handle_integer_cast();
    }

    if (*expr_ktype == *target_ktype) {
        return expr->gen();
    }

    if (expr_ktype->is_pointer() && target_ktype->is_pointer()) {
        // Pointer to pointer casts are always allowed - just bitcast the pointer
        auto* expr_val = expr->gen();
        auto* target_llvm_type = get_llvm_type(target_ktype, compiler);
        return compiler.get_builder().CreateBitCast(expr_val, target_llvm_type);
    }

    throw_incompatible_cast_error(expr_ktype, target_ktype);
    return nullptr; // Unreachable
}

llvm::Value* CastNode::gen_ptr() const
{
    return const_cast<CastNode*>(this)->expr->gen_ptr();
}

llvm::Type* CastNode::gen_type() const
{
    return get_llvm_type(type, compiler);
}

KType* CastNode::get_ktype() const
{
    return type;
}

llvm::Value* CastNode::trivial_gen()
{
    return nullptr;
}

bool CastNode::is_trivially_evaluable() const
{
    return false;
}
