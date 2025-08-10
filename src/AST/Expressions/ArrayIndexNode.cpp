#include "kyoto/AST/Expressions/ArrayIndexNode.h"

#include <format>
#include <stdexcept>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

ArrayIndexNode::ArrayIndexNode(ExpressionNode* array, ExpressionNode* index, ModuleCompiler& compiler)
    : array(array)
    , index(index)
    , compiler(compiler)
{
}

ArrayIndexNode::~ArrayIndexNode()
{
    delete array;
    delete index;
    delete cached_type;
}

std::string ArrayIndexNode::to_string() const
{
    return std::format("{}[{}]", array->to_string(), index->to_string());
}

llvm::Value* ArrayIndexNode::gen()
{
    validate_index_type();

    auto* array_ktype = array->get_ktype();

    if (array_ktype->is_array()) {
        return gen_array_access();
    } else if (array_ktype->is_pointer()) {
        return gen_pointer_access();
    } else {
        throw std::runtime_error(std::format("Cannot index into type '{}'", array_ktype->to_string()));
    }
}

llvm::Value* ArrayIndexNode::gen_ptr() const
{
    const_cast<ArrayIndexNode*>(this)->validate_index_type();

    auto* array_ktype = array->get_ktype();
    auto* index_val = index->gen();

    if (array_ktype->is_array()) {
        auto* array_ptr = array->gen_ptr();
        if (!array_ptr) {
            throw std::runtime_error("Cannot get pointer to array");
        }

        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(compiler.get_context()), 0);
        return compiler.get_builder().CreateGEP(array->gen_type(), array_ptr, { zero, index_val }, "arrayptr");
    } else if (array_ktype->is_pointer()) {
        auto* array_val = array->gen();
        return compiler.get_builder().CreateGEP(gen_type(), array_val, index_val, "arrayptr");
    } else {
        throw std::runtime_error(std::format("Cannot get pointer to element of type '{}'", array_ktype->to_string()));
    }
}

llvm::Type* ArrayIndexNode::gen_type() const
{
    auto* result_ktype = get_ktype();
    return ASTNode::get_llvm_type(result_ktype, compiler);
}

KType* ArrayIndexNode::get_ktype() const
{
    if (cached_type) {
        return cached_type;
    }

    cached_type = calculate_result_type();
    return cached_type;
}

llvm::Value* ArrayIndexNode::trivial_gen()
{
    return nullptr;
}

bool ArrayIndexNode::is_trivially_evaluable() const
{
    return false;
}

std::vector<ASTNode*> ArrayIndexNode::get_children() const
{
    return { array, index };
}

llvm::Value* ArrayIndexNode::gen_array_access() const
{
    auto* element_ptr = gen_ptr();
    return compiler.get_builder().CreateLoad(gen_type(), element_ptr, "arrayload");
}

llvm::Value* ArrayIndexNode::gen_pointer_access() const
{
    auto* element_ptr = gen_ptr();
    return compiler.get_builder().CreateLoad(gen_type(), element_ptr, "arrayload");
}

void ArrayIndexNode::validate_index_type() const
{
    auto* index_ktype = index->get_ktype();
    if (!index_ktype->is_integer()) {
        throw std::runtime_error(std::format("Array index must be an integer, got '{}'", index_ktype->to_string()));
    }
}

KType* ArrayIndexNode::calculate_result_type() const
{
    auto* array_ktype = array->get_ktype();

    if (array_ktype->is_array()) {
        return array_ktype->as<ArrayType>()->get_element_type()->copy();
    } else if (array_ktype->is_pointer()) {
        return array_ktype->as<PointerType>()->get_pointee()->copy();
    } else {
        throw std::runtime_error(std::format("Cannot index into type '{}'", array_ktype->to_string()));
    }
}
