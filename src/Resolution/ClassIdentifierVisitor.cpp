#include "kyoto/Resolution/ClassIdentifierVisitor.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/ModuleCompiler.h"

ClassIdentifierVisitor::ClassIdentifierVisitor(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

void ClassIdentifierVisitor::visit(ASTNode* node)
{
    if (!node) return;

    const auto children = node->get_children();
    for (auto* child : children) {
        if (auto* c = dynamic_cast<ClassDefinitionNode*>(child); c) {
            visit(c);
        } else {
            visit(child);
        }
    }
}

void ClassIdentifierVisitor::visit(ClassDefinitionNode* node)
{
    auto name = node->get_name();
    auto children = node->get_children();

    auto llvm_types = std::vector<llvm::Type*>();

    // Conservative estimate. children.size() is methods + fields
    llvm_types.reserve(children.size());
    for (const auto* child : children) {
        if (auto* c = dynamic_cast<const DeclarationStatementNode*>(child); c) {
            llvm_types.push_back(ASTNode::get_llvm_type(c->get_ktype(), compiler));
        }
    }

    auto* llvm_struct = llvm::StructType::create(compiler.get_context(), llvm_types, name);
    compiler.add_llvm_struct(name, llvm_struct);
}
