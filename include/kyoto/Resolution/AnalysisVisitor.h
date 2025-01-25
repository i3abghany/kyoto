#pragma once

#include "kyoto/AST/ASTNode.h"

class IAnalysisVisitor {
public:
    virtual ~IAnalysisVisitor() = default;
    virtual void visit(ASTNode* node) = 0;
};

template <typename Derived, typename NodeToVisit> class AnalysisVisitor : public IAnalysisVisitor {
public:
    void visit(ASTNode* node)
    {
        if (!node) return;

        const auto children = node->get_children();
        for (auto* child : children) {
            if (auto* n = dynamic_cast<NodeToVisit*>(child); n) static_cast<Derived*>(this)->visit(n);
            else visit(child);
        }
    }

    virtual void visit(NodeToVisit* node) = 0;
};