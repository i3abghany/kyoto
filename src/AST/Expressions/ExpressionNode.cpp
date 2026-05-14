#include "kyoto/AST/Expressions/ExpressionNode.h"

#include <assert.h>
#include <format>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
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

bool ExpressionNode::can_convert_array_to_slice(const KType* target_type, const KType* expr_type)
{
    if (!target_type->is_slice() || !expr_type->is_array()) return false;

    const auto* slice_type = target_type->as<SliceType>();
    const auto* array_type = expr_type->as<ArrayType>();
    return *slice_type->get_element_type() == *array_type->get_element_type();
}

llvm::Value* ExpressionNode::convert_array_to_slice(ExpressionNode* expr, const KType* target_type,
                                                    ModuleCompiler& compiler)
{
    if (!can_convert_array_to_slice(target_type, expr->get_ktype())) {
        throw std::runtime_error(std::format("Cannot convert `{}` from `{}` to `{}`", expr->to_string(),
                                             expr->get_ktype()->to_string(), target_type->to_string()));
    }

    auto* array_type = expr->get_ktype()->as<ArrayType>();
    auto* slice_llvm_type = ASTNode::get_llvm_type(target_type, compiler);
    auto* array_ptr = expr->gen_ptr();
    if (!array_ptr) {
        throw std::runtime_error(
            std::format("Cannot convert `{}` to slice because it is not addressable", expr->to_string()));
    }

    auto* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(compiler.get_context()), 0);
    auto* data_ptr = compiler.get_builder().CreateGEP(expr->gen_type(), array_ptr, { zero, zero }, "sliceptr");
    auto* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(compiler.get_context()), array_type->get_size(), true);

    llvm::Value* slice_value = llvm::UndefValue::get(slice_llvm_type);
    slice_value = compiler.get_builder().CreateInsertValue(slice_value, data_ptr, { 0 }, "slice.data");
    return compiler.get_builder().CreateInsertValue(slice_value, size, { 1 }, "slice.size");
}
