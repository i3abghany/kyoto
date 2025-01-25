#pragma once

#include "kyoto/Resolution/AnalysisVisitor.h"

class ASTNode;
class ClassDefinitionNode;
class ModuleCompiler;

class ClassIdentifierVisitor : public AnalysisVisitor {
public:
    explicit ClassIdentifierVisitor(ModuleCompiler& compiler);
    void visit(ASTNode* node) override;
    void visit(ClassDefinitionNode* node);

private:
    ModuleCompiler& compiler;
};