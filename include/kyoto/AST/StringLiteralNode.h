#pragma once

#include <string>

#include "kyoto/AST/ExpressionNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class StringLiteralNode final : public ExpressionNode {
public:
    StringLiteralNode(std::string value, ModuleCompiler& compiler);
    ~StringLiteralNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    void cast_to(PrimitiveType::Kind target_type);

private:
    mutable KType* type;
    std::string value;
    ModuleCompiler& compiler;
};