#pragma once

#include <string>

#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;
class KType;

namespace llvm {
class Value;
}

class ArrayIndexNode : public ExpressionNode {
public:
    ArrayIndexNode(ExpressionNode* array, ExpressionNode* index, ModuleCompiler& compiler);
    ~ArrayIndexNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

    [[nodiscard]] ExpressionNode* get_array() const { return array; }
    [[nodiscard]] ExpressionNode* get_index() const { return index; }

private:
    llvm::Value* gen_array_access() const;
    llvm::Value* gen_pointer_access() const;
    void validate_index_type() const;
    KType* calculate_result_type() const;

private:
    ExpressionNode* array;
    ExpressionNode* index;
    ModuleCompiler& compiler;
    mutable KType* cached_type = nullptr;
};
