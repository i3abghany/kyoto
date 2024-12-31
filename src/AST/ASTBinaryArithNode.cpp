#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <string>

#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"

#define ARITH_BINARY_NODE_IMPL(name, op, llvm_op)                                                               \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                              \
        : lhs(lhs)                                                                                              \
        , rhs(rhs)                                                                                              \
        , compiler(compiler)                                                                                    \
    {                                                                                                           \
    }                                                                                                           \
    std::string name::to_string() const                                                                         \
    {                                                                                                           \
        return fmt::format("{}Node({}, {})", #name, lhs->to_string(), rhs->to_string());                        \
    }                                                                                                           \
    llvm::Value* name::gen()                                                                                    \
    {                                                                                                           \
        auto* lhs_val = lhs->gen();                                                                             \
        auto* rhs_val = rhs->gen();                                                                             \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(compiler.get_context()));                  \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(compiler.get_context()));                  \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        if (!t.has_value())                                                                                     \
            assert(false && "Binary arithmetic operation type mismatch");                                       \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                     \
    }                                                                                                           \
    llvm::Type* name::get_type(llvm::LLVMContext& context) const                                                \
    {                                                                                                           \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(context));                                 \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(context));                                 \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        if (!t.has_value())                                                                                     \
            assert(false && "Binary arithmetic operation type mismatch");                                       \
        auto ktype = new PrimitiveType(t.value());                                                              \
        auto res = ASTNode::get_llvm_type(ktype, context);                                                      \
        delete ktype;                                                                                           \
        return res;                                                                                             \
    }

ARITH_BINARY_NODE_IMPL(MulNode, Mul, CreateMul);
ARITH_BINARY_NODE_IMPL(AddNode, Add, CreateAdd);
ARITH_BINARY_NODE_IMPL(SubNode, Sub, CreateSub);
ARITH_BINARY_NODE_IMPL(DivNode, Div, CreateSDiv);
ARITH_BINARY_NODE_IMPL(ModNode, Mod, CreateSRem);

#undef ARITH_BINARY_NODE_IMPL