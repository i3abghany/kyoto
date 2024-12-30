#pragma once

#include <string>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

namespace llvm {
class Value;
}

class NumberNode : public ExpressionNode {
    int64_t value;
    std::unique_ptr<KType> type;
    ModuleCompiler& compiler;

public:
    NumberNode(int64_t value, KType* ktype, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;
    llvm::Type* get_type(llvm::LLVMContext& context) const override;
};
