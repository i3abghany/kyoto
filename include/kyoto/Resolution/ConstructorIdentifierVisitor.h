#pragma once

#include "kyoto/Resolution/AnalysisVisitor.h"

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"

class ModuleCompiler;

class ConstructorIdentifierVisitor : public AnalysisVisitor<ConstructorIdentifierVisitor, FunctionCall> {
public:
    explicit ConstructorIdentifierVisitor(ModuleCompiler& compiler)
        : compiler(compiler)
    {
    }

    void visit(FunctionCall* node) override
    {
        if (const auto& name = node->get_name(); compiler.class_exists(name)) node->set_as_constructor_call();

        for (auto* arg : node->get_children()) {
            auto* f = dynamic_cast<FunctionCall*>(arg);
            if (f) visit(f);
            else AnalysisVisitor::visit(arg);
        }
    }

private:
    ModuleCompiler& compiler;
};