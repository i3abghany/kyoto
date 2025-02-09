#include "kyoto/AST/Expressions/ArrayNode.h"

#include <stddef.h>

#include "kyoto/AST/ASTNode.h"

std::string ArrayNode::to_string() const
{
    std::string str = "[";
    for (size_t i = 0; i < elements.size(); i++) {
        str += elements[i]->to_string();
        if (i != elements.size() - 1) str += ", ";
    }
    str += "]";
    return str;
}

llvm::Value* ArrayNode::gen()
{
    return nullptr;
}

llvm::Type* ArrayNode::gen_type() const
{
    return nullptr;
}

std::vector<ASTNode*> ArrayNode::get_children() const
{
    std::vector<ASTNode*> children;
    for (const auto element : elements) {
        children.push_back(element);
    }
    return children;
}