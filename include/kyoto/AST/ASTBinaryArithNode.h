#pragma once

#include <kyoto/AST/ASTNode.h>
#include <string>

class ModuleCompiler;

namespace llvm {
class Value;
}

#define ARITH_BINARY_NODE_INTERFACE(name)                           \
    class name : public ASTNode {                                   \
        ASTNode *lhs, *rhs;                                         \
        ModuleCompiler& compiler;                                   \
                                                                    \
    public:                                                         \
        name(ASTNode* lhs, ASTNode* rhs, ModuleCompiler& compiler); \
        std::string to_string() const override;                     \
        llvm::Value* gen() override;                                \
    }

ARITH_BINARY_NODE_INTERFACE(AddNode);
ARITH_BINARY_NODE_INTERFACE(SubNode);
ARITH_BINARY_NODE_INTERFACE(MulNode);
ARITH_BINARY_NODE_INTERFACE(DivNode);
ARITH_BINARY_NODE_INTERFACE(ModNode);
