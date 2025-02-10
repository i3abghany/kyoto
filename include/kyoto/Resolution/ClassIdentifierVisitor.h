#pragma once

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/Resolution/AnalysisVisitor.h"

class ClassIdentifierVisitor : public AnalysisVisitor<ClassIdentifierVisitor, ClassDefinitionNode> {
public:
    explicit ClassIdentifierVisitor(ModuleCompiler& compiler)
        : compiler(compiler)
    {
    }

    void visit(ClassDefinitionNode* node) override
    {
        auto name = node->get_name();
        auto children = node->get_children();

        auto llvm_types = std::vector<llvm::Type*>();

        // Conservative estimate. children.size() is methods + fields
        llvm_types.reserve(children.size());
        for (const auto* child : children) {
            if (const auto* c = dynamic_cast<const DeclarationStatementNode*>(child); c) {
                llvm_types.push_back(ASTNode::get_llvm_type(c->get_ktype(), compiler));
            }
        }

        auto* llvm_struct = llvm::StructType::create(compiler.get_context(), llvm_types, name);
        compiler.add_class_metadata(name, { llvm_struct, node });
    }

private:
    ModuleCompiler& compiler;
};