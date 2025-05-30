#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

class ClassDefinitionNode : public ASTNode {
public:
    ClassDefinitionNode(std::string name, std::string parent, std::vector<ASTNode*> components,
                        ModuleCompiler& compiler);
    ~ClassDefinitionNode();
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] const ASTNode* get_declaration_of(const std::string& name) const;

    [[nodiscard]] std::string get_name() const { return name; }
    [[nodiscard]] std::string get_parent() const { return parent; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return components; }

private:
    std::string name;
    std::string parent;
    std::vector<ASTNode*> components;
    ModuleCompiler& compiler;
};

class ConstructorNode : public FunctionNode {
public:
    ConstructorNode(std::string name, std::vector<FunctionNode::Parameter> args, ASTNode* body,
                    ModuleCompiler& compiler);
    ~ConstructorNode() override;

    [[nodiscard]] llvm::Value* gen() override;
};