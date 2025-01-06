#pragma once

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class StringLiteralNode : public ExpressionNode {
public:
    StringLiteralNode(std::string value, ModuleCompiler& compiler);
    ~StringLiteralNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    void cast_to(PrimitiveType::Kind target_type);

private:
    std::string value;
    ModuleCompiler& compiler;
};