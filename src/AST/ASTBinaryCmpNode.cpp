#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <string>

#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"

#define CMP_BINARY_NODE_IMPL(name, op, llvm_sop)                                                              \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                            \
        : lhs(lhs)                                                                                            \
        , rhs(rhs)                                                                                            \
        , compiler(compiler)                                                                                  \
    {                                                                                                         \
    }                                                                                                         \
    name::~name()                                                                                             \
    {                                                                                                         \
        delete lhs;                                                                                           \
        delete rhs;                                                                                           \
    }                                                                                                         \
    std::string name::to_string() const                                                                       \
    {                                                                                                         \
        return fmt::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                          \
    }                                                                                                         \
    llvm::Value* name::gen()                                                                                  \
    {                                                                                                         \
        auto* lhs_val = lhs->gen();                                                                           \
        auto* rhs_val = rhs->gen();                                                                           \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(compiler.get_context()));                \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(compiler.get_context()));                \
        auto t = compiler.get_type_resolver().resolve_binary_cmp(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        if (!t.has_value()) assert(false && "Binary comparison operation type mismatch");                     \
        auto* check = compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                           \
        return compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                                  \
    }                                                                                                         \
    llvm::Type* name::get_type(llvm::LLVMContext& context) const                                              \
    {                                                                                                         \
        return llvm::Type::getInt1Ty(context);                                                                \
    }                                                                                                         \
    bool name::is_trivially_evaluable() const                                                                 \
    {                                                                                                         \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable();                                \
    }                                                                                                         \
    llvm::Value* name::trivial_gen()                                                                          \
    {                                                                                                         \
        assert(is_trivially_evaluable());                                                                     \
        auto* lhs_val = lhs->trivial_gen();                                                                   \
        auto* rhs_val = rhs->trivial_gen();                                                                   \
        return compiler.get_builder().llvm_sop(lhs_val, rhs_val, #op "val");                                  \
    }

CMP_BINARY_NODE_IMPL(EqNode, eq, CreateICmpEQ);
CMP_BINARY_NODE_IMPL(NotEqNode, ne, CreateICmpNE);
CMP_BINARY_NODE_IMPL(LessNode, lt, CreateICmpSLT);
CMP_BINARY_NODE_IMPL(GreaterNode, gt, CreateICmpSGT);
CMP_BINARY_NODE_IMPL(LessEqNode, le, CreateICmpSLE);
CMP_BINARY_NODE_IMPL(GreaterEqNode, ge, CreateICmpSGE);

#undef CMP_BINARY_NODE_IMPL