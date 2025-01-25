#include "kyoto/Resolution/FunctionIdentifierVisitor.h"

#include <iostream>
#include <vector>

#include "kyoto/AST/ASTNode.h"

FunctionIdentifierVisitor::FunctionIdentifierVisitor(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

void FunctionIdentifierVisitor::visit(ASTNode* node)
{
    if (!node) return;

    const auto children = node->get_children();
    for (auto* child : children) {
        if (auto* c = dynamic_cast<FunctionNode*>(child); c) {
            visit(c);
        } else {
            visit(child);
        }
    }
}

void FunctionIdentifierVisitor::visit(FunctionNode* node)
{
    auto* _ = node->gen_prototype();
}