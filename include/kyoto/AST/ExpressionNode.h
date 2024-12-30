#pragma once

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

namespace llvm {
class LLVMContext;
}

namespace llvm {
class Value;
}

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    virtual llvm::Value* gen() = 0;
    virtual llvm::Type* get_type(llvm::LLVMContext& context) = 0;
};
