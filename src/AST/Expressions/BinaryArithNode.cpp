#include <assert.h>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/BinaryNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/NumberNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace {

void cast_literal_to(PrimitiveType* target_type, NumberNode* literal, ModuleCompiler& compiler)
{
    if (!target_type->is_integer()) return;
    if (!compiler.get_type_resolver().fits_in(literal->get_value(), target_type->get_kind())) {
        throw std::runtime_error(
            std::format("Value `{}` does not fit in type `{}`", literal->to_string(), target_type->to_string()));
    }
    literal->cast_to(target_type->get_kind());
}

void coerce_integer_literal_operands(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)
{
    auto* lhs_type = lhs->get_ktype()->as<PrimitiveType>();
    auto* rhs_type = rhs->get_ktype()->as<PrimitiveType>();

    if (lhs_type->is_boolean() || rhs_type->is_boolean()) return;

    if (auto* lhs_literal = dynamic_cast<NumberNode*>(lhs);
        lhs_literal && rhs_type->is_integer() && *lhs_type != *rhs_type) {
        cast_literal_to(rhs_type, lhs_literal, compiler);
        return;
    }

    if (auto* rhs_literal = dynamic_cast<NumberNode*>(rhs);
        rhs_literal && lhs_type->is_integer() && *lhs_type != *rhs_type) {
        cast_literal_to(lhs_type, rhs_literal, compiler);
    }
}

}

#define ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                                                            \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                                \
        : lhs(lhs)                                                                                                \
        , rhs(rhs)                                                                                                \
        , compiler(compiler)                                                                                      \
    {                                                                                                             \
    }                                                                                                             \
    name::~name()                                                                                                 \
    {                                                                                                             \
        delete type;                                                                                              \
        delete lhs;                                                                                               \
        delete rhs;                                                                                               \
    }                                                                                                             \
    std::string name::to_string() const                                                                           \
    {                                                                                                             \
        return std::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                              \
    }                                                                                                             \
    llvm::Value* name::gen()                                                                                      \
    {                                                                                                             \
        auto* lhs_val = lhs->gen();                                                                               \
        auto* rhs_val = rhs->gen();                                                                               \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                  \
        if (lhs_ktype->width() > rhs_ktype->width()) {                                                            \
            rhs_val = compiler.get_builder().CreateSExt(rhs_val, lhs_val->getType(), "sext");                     \
            rhs_ktype = lhs_ktype;                                                                                \
        } else if (lhs_ktype->width() < rhs_ktype->width()) {                                                     \
            lhs_val = compiler.get_builder().CreateSExt(lhs_val, rhs_val->getType(), "sext");                     \
            lhs_ktype = rhs_ktype;                                                                                \
        }                                                                                                         \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value())                                                                                       \
            throw std::runtime_error(std::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                       \
    }                                                                                                             \
    llvm::Type* name::gen_type() const                                                                            \
    {                                                                                                             \
        coerce_integer_literal_operands(lhs, rhs, compiler);                                                      \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                     \
            throw std::runtime_error(std::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                \
        }                                                                                                         \
        auto ktype = new PrimitiveType(t.value());                                                                \
        auto res = ASTNode::get_llvm_type(ktype, compiler);                                                       \
        delete ktype;                                                                                             \
        return res;                                                                                               \
    }                                                                                                             \
    KType* name::get_ktype() const                                                                                \
    {                                                                                                             \
        if (type) return type;                                                                                    \
        coerce_integer_literal_operands(lhs, rhs, compiler);                                                      \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                     \
            throw std::runtime_error(std::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                \
        }                                                                                                         \
        return type = new PrimitiveType(t.value());                                                               \
    }

#define ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(name, op, llvm_op)            \
    ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                             \
    llvm::Value* name::trivial_gen()                                           \
    {                                                                          \
        assert(is_trivially_evaluable());                                      \
        auto* lhs_val = lhs->trivial_gen();                                    \
        auto* rhs_val = rhs->trivial_gen();                                    \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");    \
    }                                                                          \
    bool name::is_trivially_evaluable() const                                  \
    {                                                                          \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable(); \
    }

#define ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(name, op, llvm_op)              \
    ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                             \
    llvm::Value* name::trivial_gen()                                           \
    {                                                                          \
        assert(is_trivially_evaluable());                                      \
        auto* lhs_val = lhs->trivial_gen();                                    \
        auto* rhs_val = rhs->trivial_gen();                                    \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");    \
    }                                                                          \
    bool name::is_trivially_evaluable() const                                  \
    {                                                                          \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable(); \
    }

ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(MulNode, *, CreateMul);
ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(AddNode, +, CreateAdd);
ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(SubNode, -, CreateSub);
ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(DivNode, /, CreateSDiv);
ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(ModNode, %, CreateSRem);

#undef ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL
#undef ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL
#undef ARITH_BINARY_NODE_IMPL_BASE
