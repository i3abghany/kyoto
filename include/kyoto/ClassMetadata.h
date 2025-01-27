#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/AST/DeclarationNodes.h"

namespace llvm {
class Value;
class StructType;
}

struct ClassMetadata {
    llvm::StructType* llvm_type;
    ClassDefinitionNode* node;

    [[nodiscard]] std::optional<size_t> member_idx(const std::string& name) const
    {
        for (size_t i = 0; i < node->get_children().size(); ++i) {
            if (node->get_children()[i]->is<DeclarationStatementNode>()) {
                auto* decl = node->get_children()[i]->as<DeclarationStatementNode>();
                if (decl->get_name() == name) {
                    return i;
                }
            }
        }
        return -1;
    }
};