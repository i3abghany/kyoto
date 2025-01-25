#pragma once

class ASTNode;

class AnalysisVisitor {
public:
    ~AnalysisVisitor() = default;
    virtual void visit(ASTNode* node) = 0;
};