#include "kyoto/Resolution/ConstructorResolution.h"

#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/FunctionCall.h"
#include "kyoto/ModuleCompiler.h"

ConstructorResolver::ConstructorResolver(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

void ConstructorResolver::visit(ASTNode* node)
{
    // We expect `children` to come in a specific order, so they can be null.
    // (e.g. a for loop statement with no init will have a nullptr in index 0)
    if (!node) return;

    const auto children = node->get_children();
    for (auto* child : children) {
        if (auto* f = dynamic_cast<FunctionCall*>(child); f) {
            visit(f);
        } else {
            visit(child);
        }
    }
}

void ConstructorResolver::visit(FunctionCall* node)
{
    if (const auto& name = node->get_name(); compiler.class_exists(name)) node->set_as_constructor_call();

    for (auto* arg : node->get_children()) {
        auto* f = dynamic_cast<FunctionCall*>(arg);
        visit(f ? f : arg);
    }
}