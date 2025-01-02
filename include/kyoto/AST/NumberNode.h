#pragma once

#include <stdint.h>
#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;

class NumberNode : public ExpressionNode {
    int64_t value;
    KType* type;
    ModuleCompiler& compiler;

public:
    NumberNode(int64_t value, KType* ktype, ModuleCompiler& compiler);
    ~NumberNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen();
    [[nodiscard]] bool is_trivially_evaluable() const;
};
