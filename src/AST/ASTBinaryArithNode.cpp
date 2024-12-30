#include <fmt/core.h>
#include <string>

#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/IRBuilder.h"

#include "kyoto/AST/ASTBinaryArithNode.h"
#include "kyoto/AST/ASTNode.h"

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
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                     \
    }                                                                                                           \
    llvm::Type* name::get_type(llvm::LLVMContext& context) const                                                \
    {                                                                                                           \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(context));                                 \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(context));                                 \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        auto ktype = new PrimitiveType(t);                                                                      \
        auto res = ASTNode::get_llvm_type(ktype, context);                                                      \
        delete ktype;                                                                                           \
        return res;                                                                                             \
    }

ARITH_BINARY_NODE_IMPL(MulNode, mul, CreateMul);
ARITH_BINARY_NODE_IMPL(AddNode, add, CreateAdd);
ARITH_BINARY_NODE_IMPL(SubNode, sub, CreateSub);
ARITH_BINARY_NODE_IMPL(DivNode, div, CreateSDiv);
ARITH_BINARY_NODE_IMPL(ModNode, mod, CreateSRem);
