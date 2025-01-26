#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;
class ExpressionNode;

namespace llvm {
class AllocaInst;
class Value;
}

class DeclarationStatementNode final : public ASTNode {
public:
    DeclarationStatementNode(std::string name, KType* ktype, ModuleCompiler& compiler);
    ~DeclarationStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

    [[nodiscard]] KType* get_ktype() const { return type; }
    [[nodiscard]] std::string get_name() const { return name; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return {}; }

private:
    std::string name;
    KType* type;
    ModuleCompiler& compiler;
};

class FullDeclarationStatementNode final : public ASTNode {
public:
    FullDeclarationStatementNode(std::string name, KType* type, ExpressionNode* expr, ModuleCompiler& compiler);
    ~FullDeclarationStatementNode() override;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] KType* get_ktype() const { return type; }
    [[nodiscard]] std::string get_name() const { return name; }
    [[nodiscard]] ExpressionNode* get_expression() const { return expr; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override;

private:
    void initialize_type();
    void validate_type_not_void() const;
    void validate_expression_not_void() const;
    llvm::AllocaInst* create_alloca() const;
    llvm::Value* generate_expression_value(llvm::AllocaInst* alloca);
    bool is_assigning_to_class_instance() const;
    llvm::Value* handle_constructor_call(llvm::AllocaInst* alloca) const;
    void store_val_and_register_symbol(llvm::Value* expr_val, llvm::AllocaInst* alloca) const;

private:
    std::string name;
    KType* type;
    ExpressionNode* expr;
    ModuleCompiler& compiler;
};
