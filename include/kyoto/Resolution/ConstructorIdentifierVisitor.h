#pragma once

#include "kyoto/Resolution/AnalysisVisitor.h"

class ASTNode;
class FunctionCall;
class ModuleCompiler;

class ConstructorIdentifierVisitor : public AnalysisVisitor {
public:
    explicit ConstructorIdentifierVisitor(ModuleCompiler& compiler);

    void visit(ASTNode* node) override;
    void visit(FunctionCall* node);

private:
    ModuleCompiler& compiler;
};