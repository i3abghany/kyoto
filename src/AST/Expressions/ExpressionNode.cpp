#include "kyoto/AST/Expressions/ExpressionNode.h"

#include <assert.h>
#include <format>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Casting.h"

namespace llvm {
class Value;
}

void ExpressionNode::check_boolean_promotion(const PrimitiveType* expr_ktype, const PrimitiveType* target_type,
                                             const std::string& target_name)
{
    if (expr_ktype->is_boolean() && !target_type->is_boolean()) {
        throw std::runtime_error(std::format("Cannot convert value of type `{}` to `{}`{}", expr_ktype->to_string(),
                                             target_type->to_string(),
                                             target_name.empty() ? "" : std::format(" for `{}`", target_name)));
    }
}

void ExpressionNode::check_int_range_fit(int64_t val, const PrimitiveType* target_type, ModuleCompiler& compiler,
                                         const std::string& expr, const std::string& target_name)
{
    if (!compiler.get_type_resolver().fits_in(val, target_type->get_kind())) {
        throw std::runtime_error(std::format("Value of RHS `{}` = `{}` does not fit in type `{}`{}", expr, val,
                                             target_type->to_string(),
                                             target_name.empty() ? "" : std::format(" for `{}`", target_name)));
    }
}

llvm::Value* ExpressionNode::promoted_trivially_gen(ExpressionNode* expr, ModuleCompiler& compiler,
                                                    const KType* target_ktype, const std::string& target_name)
{
    if (!expr->is_trivially_evaluable()) return nullptr;

    auto* target_type = target_ktype->as<PrimitiveType>();
    auto* expr_ktype = expr->get_ktype()->as<PrimitiveType>();
    check_boolean_promotion(expr_ktype, target_type, target_name);

    const auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(expr->trivial_gen());
    assert(constant_int && "Trivial value must be a constant int");
    const auto int_val = target_type->is_boolean() || expr_ktype->is_boolean() ? constant_int->getZExtValue()
                                                                               : constant_int->getSExtValue();

    check_int_range_fit(int_val, target_type, compiler, expr->to_string(), target_name);
    return llvm::ConstantInt::get(get_llvm_type(target_type, compiler), int_val, true);
}

llvm::Value* ExpressionNode::dynamic_integer_conversion(llvm::Value* expr_val, const PrimitiveType* expr_ktype,
                                                        const PrimitiveType* target_type, ModuleCompiler& compiler)
{
    auto* ltype = get_llvm_type(target_type, compiler);

    if (expr_ktype->width() > target_type->width()) {
        return compiler.get_builder().CreateTrunc(expr_val, ltype);
    }
    if (expr_ktype->width() < target_type->width()) {
        return compiler.get_builder().CreateSExt(expr_val, ltype);
    }

    return expr_val;
}

llvm::Value* ExpressionNode::handle_integer_conversion(ExpressionNode* expr, const KType* target_ktype,
                                                       ModuleCompiler& compiler, const std::string& what,
                                                       const std::string& target_name)
{
    auto* target_type = dynamic_cast<const PrimitiveType*>(target_ktype);
    auto* expr_ktype = expr->get_ktype()->as<PrimitiveType>();
    bool is_compatible = compiler.get_type_resolver().promotable_to(expr_ktype->get_kind(), target_type->get_kind());
    bool is_trivially_evaluable = expr->is_trivially_evaluable();

    if (!is_compatible && !is_trivially_evaluable) {
        throw std::runtime_error(std::format("Cannot {} value of type `{}`. Expected `{}`{}", what,
                                             expr_ktype->to_string(), target_type->to_string(),
                                             target_name.empty() ? "" : std::format(" for `{}`", target_name)));
    }

    if (auto* trivial = promoted_trivially_gen(expr, compiler, target_ktype, target_name); trivial) {
        return trivial;
    }

    if (is_compatible) {
        return dynamic_integer_conversion(expr->gen(), expr_ktype, target_type, compiler);
    }

    assert(false && "Unreachable");
}
