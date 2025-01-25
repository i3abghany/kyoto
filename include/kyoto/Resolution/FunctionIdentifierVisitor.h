#pragma once

#include "kyoto/AST/ASTNode.h"
#include "kyoto/Resolution/AnalysisVisitor.h"

class ModuleCompiler;

class FunctionIdentifierVisitor : public AnalysisVisitor<FunctionIdentifierVisitor, FunctionNode> {
public:
    explicit FunctionIdentifierVisitor(ModuleCompiler& compiler)
        : compiler(compiler)
    {
    }

    void visit(FunctionNode* node) override { auto* _ = node->gen_prototype(); }

private:
    ModuleCompiler& compiler;
};