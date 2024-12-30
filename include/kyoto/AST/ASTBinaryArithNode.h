#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

namespace llvm {
class Value;
}

#define ARITH_BINARY_NODE_INTERFACE(name)                                         \
    class name : public ExpressionNode {                                          \
        ExpressionNode *lhs, *rhs;                                                \
        ModuleCompiler& compiler;                                                 \
                                                                                  \
    public:                                                                       \
        name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler); \
        std::string to_string() const override;                                   \
        llvm::Value* gen() override;                                              \
        llvm::Type* get_type(llvm::LLVMContext& context) const override;          \
    }

ARITH_BINARY_NODE_INTERFACE(AddNode);
ARITH_BINARY_NODE_INTERFACE(SubNode);
ARITH_BINARY_NODE_INTERFACE(MulNode);
ARITH_BINARY_NODE_INTERFACE(DivNode);
ARITH_BINARY_NODE_INTERFACE(ModNode);
