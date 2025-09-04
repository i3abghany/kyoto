#include "kyoto/AST/Expressions/CastNode.h"

#include <format>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"

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
        // TODO: implement pointer casts.
        throw_incompatible_cast_error(expr_ktype, target_ktype);
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
