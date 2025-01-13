#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

class ClassDefinitionNode : public ASTNode {
public:
    ClassDefinitionNode(std::string name, std::vector<ASTNode*> components, ModuleCompiler& compiler);
    ~ClassDefinitionNode();
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;

private:
    std::string name;
    std::vector<ASTNode*> components;
    ModuleCompiler& compiler;
};