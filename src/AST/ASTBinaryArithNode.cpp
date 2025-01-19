#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ExpressionNode.h"
#include "kyoto/AST/NumberNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

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
        return fmt::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                              \
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
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                       \
    }                                                                                                             \
    llvm::Type* name::gen_type() const                                                                            \
    {                                                                                                             \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                  \
        if (auto* num = dynamic_cast<NumberNode*>(lhs); num) {                                                    \
            num->cast_to(rhs_ktype->get_kind());                                                                  \
            lhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                    \
        } else if (auto* num = dynamic_cast<NumberNode*>(rhs); num) {                                             \
            num->cast_to(lhs_ktype->get_kind());                                                                  \
            rhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                    \
        }                                                                                                         \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                     \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
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
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                  \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                     \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,   \
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
