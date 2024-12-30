#include <fmt/core.h>
#include <string>

#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/IRBuilder.h"

#include "kyoto/AST/ASTBinaryArithNode.h"
#include "kyoto/AST/ASTNode.h"

#define ARITH_BINARY_NODE_IMPL(name, op, llvm_op)                                                                      \
    name::name(ASTNode* lhs, ASTNode* rhs, ModuleCompiler& compiler)                                                   \
        : lhs(lhs)                                                                                                     \
        , rhs(rhs)                                                                                                     \
        , compiler(compiler)                                                                                           \
    {                                                                                                                  \
    }                                                                                                                  \
    std::string name::to_string() const                                                                                \
    {                                                                                                                  \
        return fmt::format("{}Node({}, {})", #name, lhs->to_string(), rhs->to_string());                               \
    }                                                                                                                  \
    llvm::Value* name::gen()                                                                                           \
    {                                                                                                                  \
        auto* lhs_val = lhs->gen();                                                                                    \
        auto* rhs_val = rhs->gen();                                                                                    \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                            \
    }

ARITH_BINARY_NODE_IMPL(MulNode, "mul", CreateMul);
ARITH_BINARY_NODE_IMPL(AddNode, "add", CreateAdd);
ARITH_BINARY_NODE_IMPL(SubNode, "sub", CreateSub);
ARITH_BINARY_NODE_IMPL(DivNode, "div", CreateSDiv);
ARITH_BINARY_NODE_IMPL(ModNode, "mod", CreateSRem);
