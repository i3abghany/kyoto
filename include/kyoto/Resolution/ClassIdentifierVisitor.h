#pragma once

class ASTNode;
class ClassDefinitionNode;
class ModuleCompiler;

class ClassIdentifierVisitor {
public:
    explicit ClassIdentifierVisitor(ModuleCompiler& compiler);
    void visit(ASTNode* node);
    void visit(ClassDefinitionNode* node);

private:
    ModuleCompiler& compiler;
};