#include "kyoto/AST/Expressions/ArrayNode.h"

#include <format>
#include <llvm/IR/Type.h>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

ArrayNode::ArrayNode(std::vector<ExpressionNode*> elements, KType* type, ModuleCompiler& compiler)
    : elements(std::move(elements))
    , type(type)
    , compiler(compiler)
{
    check_types();
}

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

void ArrayNode::check_types() const
{
    if (elements.empty()) return;

    const auto* elem_ktype = get_ktype()->as<ArrayType>()->get_element_type();
    const auto* first_elem_type = elements[0]->get_ktype();

    for (const auto& elem : elements) {
        const auto* elem_type = elem->get_ktype();

        const auto* expected_type = elem_ktype->is_array() ? first_elem_type : elem_ktype;

        if (!elem_type->operator==(*expected_type)) {
            throw std::runtime_error(std::format("Array element type mismatch: expected {}, got {}",
                                                 expected_type->to_string(), elem_type->to_string()));
        }
    }
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
    // FIXME: This is a temporary solution as there's no "easy" way
    // to get the type of an element if there are no elements.
    if (elements.empty()) return llvm::ArrayType::get(llvm::Type::getInt8Ty(compiler.get_context()), 0);
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