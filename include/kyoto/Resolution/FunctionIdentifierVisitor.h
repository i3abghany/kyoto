#pragma once

#include "kyoto/Resolution/AnalysisVisitor.h"

class ASTNode;
class FunctionNode;
class ModuleCompiler;

class FunctionIdentifierVisitor : public AnalysisVisitor {
public:
    explicit FunctionIdentifierVisitor(ModuleCompiler& compiler);
    void visit(ASTNode* node) override;
    void visit(FunctionNode* node);

private:
    ModuleCompiler& compiler;
};