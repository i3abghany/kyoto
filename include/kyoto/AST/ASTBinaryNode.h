#pragma once

#include <string>

#include "kyoto/AST/ExpressionNode.h"

class ModuleCompiler;

namespace llvm {
class Value;
}

#define BINARY_NODE_INTERFACE(name)                                                                \
    class name : public ExpressionNode {                                                           \
        ExpressionNode *lhs, *rhs;                                                                 \
        mutable KType* type = nullptr;                                                             \
        ModuleCompiler& compiler;                                                                  \
                                                                                                   \
    public:                                                                                        \
        name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler);                  \
        ~name();                                                                                   \
        std::string to_string() const override;                                                    \
        llvm::Value* gen() override;                                                               \
        llvm::Type* gen_type() const override;                                                     \
        [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { lhs, rhs }; } \
        KType* get_ktype() const override;                                                         \
        llvm::Value* trivial_gen() override;                                                       \
        bool is_trivially_evaluable() const override;                                              \
    }

BINARY_NODE_INTERFACE(AddNode);
BINARY_NODE_INTERFACE(SubNode);
BINARY_NODE_INTERFACE(MulNode);
BINARY_NODE_INTERFACE(DivNode);
BINARY_NODE_INTERFACE(ModNode);

BINARY_NODE_INTERFACE(EqNode);
BINARY_NODE_INTERFACE(NotEqNode);
BINARY_NODE_INTERFACE(LessNode);
BINARY_NODE_INTERFACE(GreaterNode);
BINARY_NODE_INTERFACE(LessEqNode);
BINARY_NODE_INTERFACE(GreaterEqNode);

BINARY_NODE_INTERFACE(LogicalAndNode);
BINARY_NODE_INTERFACE(LogicalOrNode);

#undef BINARY_NODE_INTERFACE