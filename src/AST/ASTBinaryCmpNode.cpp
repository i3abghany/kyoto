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
#include "llvm/IR/Value.h"

#define CMP_BINARY_NODE_IMPL(name, op, llvm_sop)                                                                \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                              \
        : lhs(lhs)                                                                                              \
        , rhs(rhs)                                                                                              \
        , compiler(compiler)                                                                                    \
    {                                                                                                           \
    }                                                                                                           \
    name::~name()                                                                                               \
    {                                                                                                           \
        delete lhs;                                                                                             \
        delete rhs;                                                                                             \
    }                                                                                                           \
    std::string name::to_string() const                                                                         \
    {                                                                                                           \
        return fmt::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                            \
    }                                                                                                           \
    llvm::Value* name::gen()                                                                                    \
    {                                                                                                           \
        auto* lhs_val = lhs->gen();                                                                             \
        auto* rhs_val = rhs->gen();                                                                             \
        auto* lhs_ktype = KType::from_llvm_type(lhs->get_type(compiler.get_context()))->as<PrimitiveType>();    \
        auto* rhs_ktype = KType::from_llvm_type(rhs->get_type(compiler.get_context()))->as<PrimitiveType>();    \
        if (lhs_ktype->width() > rhs_ktype->width()) {                                                          \
            rhs_val = compiler.get_builder().CreateSExt(rhs_val, lhs_val->getType(), "sext");                   \
            rhs_ktype = KType::from_llvm_type(lhs_val->getType())->as<PrimitiveType>();                         \
        } else if (lhs_ktype->width() < rhs_ktype->width()) {                                                   \
            lhs_val = compiler.get_builder().CreateSExt(lhs_val, rhs_val->getType(), "sext");                   \
            lhs_ktype = KType::from_llvm_type(rhs_val->getType())->as<PrimitiveType>();                         \
        }                                                                                                       \
        auto t = compiler.get_type_resolver().resolve_binary_cmp(lhs_ktype->get_kind(), rhs_ktype->get_kind()); \
        if (!t.has_value()) {                                                                                   \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op, \
                                                 lhs_ktype->to_string(), rhs_ktype->to_string()));              \
        }                                                                                                       \
        auto* check = compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                             \
        return compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                                    \
    }                                                                                                           \
    llvm::Type* name::get_type(llvm::LLVMContext& context) const                                                \
    {                                                                                                           \
        return llvm::Type::getInt1Ty(context);                                                                  \
    }                                                                                                           \
    bool name::is_trivially_evaluable() const                                                                   \
    {                                                                                                           \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable();                                  \
    }                                                                                                           \
    llvm::Value* name::trivial_gen()                                                                            \
    {                                                                                                           \
        assert(is_trivially_evaluable());                                                                       \
        auto* lhs_val = lhs->trivial_gen();                                                                     \
        auto* rhs_val = rhs->trivial_gen();                                                                     \
        return compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                                    \
    }

CMP_BINARY_NODE_IMPL(EqNode, ==, CreateICmpEQ);
CMP_BINARY_NODE_IMPL(NotEqNode, !=, CreateICmpNE);
CMP_BINARY_NODE_IMPL(LessNode, <, CreateICmpSLT);
CMP_BINARY_NODE_IMPL(GreaterNode, >, CreateICmpSGT);
CMP_BINARY_NODE_IMPL(LessEqNode, <=, CreateICmpSLE);
CMP_BINARY_NODE_IMPL(GreaterEqNode, >=, CreateICmpSGE);

#undef CMP_BINARY_NODE_IMPL