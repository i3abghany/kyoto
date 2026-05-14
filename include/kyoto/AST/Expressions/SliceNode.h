#pragma once

#include <string>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;
class KType;

class SliceNode : public ExpressionNode {
public:
    SliceNode(ExpressionNode* ptr, ExpressionNode* size, ModuleCompiler& compiler);
    ~SliceNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    void validate() const;

    ExpressionNode* ptr;
    ExpressionNode* size;
    ModuleCompiler& compiler;
    mutable KType* cached_type = nullptr;
};
