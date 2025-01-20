#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"

#define LOGICAL_BINARY_NODE_IMPL(name, op, llvm_op)                                                                 \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                                  \
        : lhs(lhs)                                                                                                  \
        , rhs(rhs)                                                                                                  \
        , compiler(compiler)                                                                                        \
    {                                                                                                               \
    }                                                                                                               \
    name::~name()                                                                                                   \
    {                                                                                                               \
        delete type;                                                                                                \
        delete lhs;                                                                                                 \
        delete rhs;                                                                                                 \
    }                                                                                                               \
    std::string name::to_string() const                                                                             \
    {                                                                                                               \
        return fmt::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                                \
    }                                                                                                               \
    llvm::Value* name::gen()                                                                                        \
    {                                                                                                               \
        auto* lhs_val = lhs->gen();                                                                                 \
        auto* rhs_val = rhs->gen();                                                                                 \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                    \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                    \
        auto t = compiler.get_type_resolver().resolve_binary_logical(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                       \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,     \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                  \
        }                                                                                                           \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                         \
    }                                                                                                               \
    llvm::Type* name::gen_type() const                                                                              \
    {                                                                                                               \
        return llvm::Type::getInt1Ty(compiler.get_context());                                                       \
    }                                                                                                               \
    bool name::is_trivially_evaluable() const                                                                       \
    {                                                                                                               \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable();                                      \
    }                                                                                                               \
    llvm::Value* name::trivial_gen()                                                                                \
    {                                                                                                               \
        assert(is_trivially_evaluable());                                                                           \
        auto* lhs_val = lhs->trivial_gen();                                                                         \
        auto* rhs_val = rhs->trivial_gen();                                                                         \
        auto* lhs_ktype = lhs->get_ktype()->as<PrimitiveType>();                                                    \
        auto* rhs_ktype = rhs->get_ktype()->as<PrimitiveType>();                                                    \
        auto t = compiler.get_type_resolver().resolve_binary_logical(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                       \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op,     \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));                  \
        }                                                                                                           \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                         \
    }                                                                                                               \
    KType* name::get_ktype() const                                                                                  \
    {                                                                                                               \
        if (type) return type;                                                                                      \
        return type = new PrimitiveType(PrimitiveType::Kind::Boolean);                                              \
    }

LOGICAL_BINARY_NODE_IMPL(LogicalAndNode, &&, CreateAnd);
LOGICAL_BINARY_NODE_IMPL(LogicalOrNode, ||, CreateOr);