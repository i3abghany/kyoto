#include "kyoto/AST/ExpressionNode.h"

#include <assert.h>
#include <fmt/core.h>
#include <memory>
#include <optional>
#include <stdexcept>

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/SymbolTable.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace llvm {
class Value;
} // namespace llvm

void ExpressionNode::check_boolean_promotion(PrimitiveType* expr_ktype, PrimitiveType* target_type,
                                             const std::string& target_name)
{
    if (expr_ktype->is_boolean() && !target_type->is_boolean()) {
        throw std::runtime_error(fmt::format("Cannot convert value of type `{}` to `{}`{}", expr_ktype->to_string(),
                                             target_type->to_string(),
                                             target_name.empty() ? "" : fmt::format(" for `{}`", target_name)));
    }
}

void ExpressionNode::check_int_range_fit(int64_t val, PrimitiveType* target_type, ModuleCompiler& compiler,
                                         const std::string& expr, const std::string& target_name)
{
    if (!compiler.get_type_resolver().fits_in(val, target_type->get_kind())) {
        throw std::runtime_error(fmt::format("Value of RHS `{}` = `{}` does not fit in type `{}`{}", expr, val,
                                             target_type->to_string(),
                                             target_name.empty() ? "" : fmt::format(" for `{}`", target_name)));
    }
}

llvm::Value* ExpressionNode::promoted_trivially_gen(ExpressionNode* expr, ModuleCompiler& compiler, KType* target_ktype,
                                                    const std::string& target_name)
{
    if (!expr->is_trivially_evaluable()) return nullptr;

    auto* target_type = dynamic_cast<PrimitiveType*>(target_ktype);
    auto expr_ktype = PrimitiveType::from_llvm_type(expr->get_type(compiler.get_context()));
    check_boolean_promotion(&expr_ktype, target_type, target_name);

    auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(expr->trivial_gen());
    assert(constant_int && "Trivial value must be a constant int");
    auto int_val = target_type->is_boolean() || expr_ktype.is_boolean() ? constant_int->getZExtValue()
                                                                        : constant_int->getSExtValue();

    check_int_range_fit(int_val, target_type, compiler, expr->to_string(), target_name);
    return llvm::ConstantInt::get(get_llvm_type(target_type, compiler.get_context()), int_val, true);
}

llvm::Value* ExpressionNode::dynamic_integer_conversion(llvm::Value* expr_val, PrimitiveType* expr_ktype,
                                                        PrimitiveType* target_type, ModuleCompiler& compiler)
{
    auto* ltype = get_llvm_type(target_type, compiler.get_context());

    if (expr_ktype->width() > target_type->width()) {
        return compiler.get_builder().CreateTrunc(expr_val, ltype);
    } else if (expr_ktype->width() < target_type->width()) {
        return compiler.get_builder().CreateSExt(expr_val, ltype);
    }

    return expr_val;
}

llvm::Value* ExpressionNode::handle_integer_conversion(ExpressionNode* expr, KType* target_ktype,
                                                       ModuleCompiler& compiler, const std::string& what,
                                                       const std::string& target_name)
{
    auto* target_type = dynamic_cast<PrimitiveType*>(target_ktype);
    auto expr_ktype = PrimitiveType::from_llvm_type(expr->get_type(compiler.get_context()));
    bool is_compatible = compiler.get_type_resolver().promotable_to(expr_ktype.get_kind(), target_type->get_kind());
    bool is_trivially_evaluable = expr->is_trivially_evaluable();

    if (!is_compatible && !is_trivially_evaluable) {
        throw std::runtime_error(fmt::format("Cannot {} value of type `{}`. Expected `{}`{}", what,
                                             expr_ktype.to_string(), target_type->to_string(),
                                             target_name.empty() ? "" : fmt::format(" for `{}`", target_name)));
    }

    if (auto* trivial = ExpressionNode::promoted_trivially_gen(expr, compiler, target_ktype, target_name); trivial) {
        return trivial;
    }

    if (is_compatible) {
        return dynamic_integer_conversion(expr->gen(), &expr_ktype, target_type, compiler);
    }

    assert(false && "Unreachable");
}

IdentifierExpressionNode::IdentifierExpressionNode(std::string name, ModuleCompiler& compiler)
    : name(name)
    , compiler(compiler)
{
}

std::string IdentifierExpressionNode::to_string() const
{
    return fmt::format("IdentifierNode({})", name);
}

llvm::Value* IdentifierExpressionNode::gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    auto symbol = symbol_opt.value();
    return compiler.get_builder().CreateLoad(symbol.alloc->getAllocatedType(), symbol.alloc, name);
}

llvm::Value* IdentifierExpressionNode::gen_ptr() const
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    auto symbol = symbol_opt.value();
    return symbol.alloc;
}

llvm::Type* IdentifierExpressionNode::get_type(llvm::LLVMContext& context) const
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }
    auto stored_type = symbol.value().kind;
    return symbol.value().alloc->getAllocatedType();
}

AssignmentNode::AssignmentNode(std::string name, ExpressionNode* expr, ModuleCompiler& compiler)
    : name(name)
    , expr(expr)
    , compiler(compiler)
{
}

AssignmentNode::~AssignmentNode()
{
    delete expr;
}

std::string AssignmentNode::to_string() const
{
    return fmt::format("AssignmentNode({}, {})", name, expr->to_string());
}

llvm::Value* AssignmentNode::gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        throw std::runtime_error(fmt::format("Unknown symbol `{}`", name));
    }

    auto symbol = symbol_opt.value();
    auto type = std::unique_ptr<KType>(new PrimitiveType(symbol.kind));

    auto* ktype = dynamic_cast<PrimitiveType*>(type.get());
    llvm::Value* expr_val = nullptr;
    auto pt = PrimitiveType::from_llvm_type(expr->get_type(compiler.get_context()));
    if (ktype->is_integer() && pt.is_integer() || ktype->is_boolean() && pt.is_boolean()) {
        expr_val = ExpressionNode::handle_integer_conversion(expr, type.get(), compiler, "assign", name);
    } else if (ktype->is_string() && pt.is_string()) {
        expr_val = expr->gen();
    } else {
        throw std::runtime_error(
            fmt::format("Type of expression `{}` (type: `{}`) can't be assigned to the variable `{}` (type `{}`)",
                        expr->to_string(), pt.to_string(), name, type->to_string()));
    }

    compiler.get_builder().CreateStore(expr_val, symbol.alloc);

    return expr_val;
}

llvm::Type* AssignmentNode::get_type(llvm::LLVMContext& context) const
{
    return expr->get_type(context);
}

llvm::Value* AssignmentNode::trivial_gen()
{
    auto symbol_opt = compiler.get_symbol(name);
    if (!symbol_opt.has_value()) {
        assert(false && "Unknown symbol");
    }

    auto symbol = symbol_opt.value();
    auto type = std::unique_ptr<KType>(new PrimitiveType(symbol.kind));
    auto* ltype = get_llvm_type(type.get(), compiler.get_context());
    auto* expr_val = ExpressionNode::handle_integer_conversion(expr, type.get(), compiler, "assign", name);
    compiler.get_builder().CreateStore(expr_val, symbol.alloc);

    return expr_val;
}

bool AssignmentNode::is_trivially_evaluable() const
{
    return expr->is_trivially_evaluable();
}

UnaryNode::UnaryNode(ExpressionNode* expr, UnaryNode::UnaryOp op, ModuleCompiler& compiler)
    : expr(expr)
    , op(op)
    , compiler(compiler)
{
}

UnaryNode::~UnaryNode()
{
    delete expr;
}

std::string UnaryNode::to_string() const
{
    return fmt::format("UnaryNode({}, {})", op_to_string(), expr->to_string());
}

llvm::Value* UnaryNode::gen()
{
    auto* expr_val = expr->gen();
    auto expr_ltype = expr->get_type(compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr_ltype);

    if (op == UnaryOp::Negate) return compiler.get_builder().CreateNeg(expr_val, "negval");
    else if (op == UnaryOp::Positive) return expr_val;
    else if (op == UnaryOp::PrefixDecrement) return gen_prefix_decrement();
    else if (op == UnaryOp::PrefixIncrement) return gen_prefix_increment();
    else assert(false && "Unknown unary operator");

    return nullptr;
}

llvm::Value* UnaryNode::gen_prefix_increment()
{
    assert(op == UnaryOp::PrefixIncrement && "Expected prefix increment unary");

    auto expr_ltype = expr->get_type(compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr_ltype);

    if (expr->is_trivially_evaluable() || !expr->gen_ptr() || !expr_ktype.is_integer()) {
        throw std::runtime_error(fmt::format("Cannot apply op `++` to {}expression `{}`",
                                             expr_ktype.is_integer() ? "" : "lvalue", expr->to_string()));
    }

    auto* expr_ptr = expr->gen_ptr();
    auto* expr_val = compiler.get_builder().CreateLoad(expr_ltype, expr_ptr, "incptr");
    assert(expr_val && "Expression value must not be null");
    auto* one = llvm::ConstantInt::get(expr_ltype, 1, true);
    auto* new_val = compiler.get_builder().CreateAdd(expr_val, one, "incval");
    compiler.get_builder().CreateStore(new_val, expr_ptr);
    return new_val;
}

llvm::Value* UnaryNode::gen_prefix_decrement()
{
    assert(op == UnaryOp::PrefixDecrement && "Expected prefix decrement unary");

    auto expr_ltype = expr->get_type(compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr_ltype);

    if (expr->is_trivially_evaluable() || !expr->gen_ptr() || !expr_ktype.is_integer()) {
        throw std::runtime_error(fmt::format("Cannot apply op `--` to {}expression `{}`",
                                             expr_ktype.is_integer() ? "" : "lvalue", expr->to_string()));
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
    auto expr_ltype = expr->get_type(compiler.get_context());
    auto expr_ktype = PrimitiveType::from_llvm_type(expr_ltype);

    if (op == UnaryOp::Negate) {
        auto* constant_int = llvm::dyn_cast<llvm::ConstantInt>(expr_val);
        assert(constant_int && "Trivial value must be a constant int");
        return llvm::ConstantInt::get(expr_ltype, -constant_int->getSExtValue(), true);
    } else if (op == UnaryOp::Positive) {
        return expr_val;
    } else {
        assert(false && "Unknown unary operator");
    }
}

bool UnaryNode::is_trivially_evaluable() const
{
    return simple_op() && expr->is_trivially_evaluable();
}

llvm::Type* UnaryNode::get_type(llvm::LLVMContext& context) const
{
    return expr->get_type(context);
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
    default:
        return "";
    }
}

bool UnaryNode::simple_op() const
{
    return op == UnaryOp::Negate || op == UnaryOp::Positive;
}
