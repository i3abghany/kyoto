#include "kyoto/AST/Expressions/ArrayNode.h"

#include <stddef.h>

#include "kyoto/AST/ASTNode.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

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
    auto* type = gen_type();
    std::vector<llvm::Constant*> eval(elements.size());
    for (size_t i = 0; i < elements.size(); i++) {
        eval[i] = llvm::dyn_cast<llvm::Constant>(elements[i]->gen());
    }
    return llvm::ConstantArray::get(static_cast<llvm::ArrayType*>(type), eval);
}

llvm::Type* ArrayNode::gen_type() const
{
    return llvm::ArrayType::get(elements[0]->gen_type(), elements.size());
}

std::vector<ASTNode*> ArrayNode::get_children() const
{
    std::vector<ASTNode*> children(elements.size());
    for (size_t i = 0; i < elements.size(); i++) {
        children[i] = elements[i];
    }
    return children;
}