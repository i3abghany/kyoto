#include "kyoto/AST/Expressions/SliceNode.h"

#include <format>
#include <stdexcept>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"

SliceNode::SliceNode(ExpressionNode* ptr, ExpressionNode* size, ModuleCompiler& compiler)
    : ptr(ptr)
    , size(size)
    , compiler(compiler)
{
}

SliceNode::~SliceNode()
{
    delete ptr;
    delete size;
    delete cached_type;
}

std::string SliceNode::to_string() const
{
    return std::format("[{}, {}]", ptr->to_string(), size->to_string());
}

llvm::Value* SliceNode::gen()
{
    validate();

    auto* slice_type = gen_type();
    PrimitiveType i64_type(PrimitiveType::Kind::I64);
    auto* ptr_value = ptr->gen();
    auto* size_value
        = ExpressionNode::handle_integer_conversion(size, &i64_type, compiler, "create slice", to_string());

    llvm::Value* slice_value = llvm::UndefValue::get(slice_type);
    slice_value = compiler.get_builder().CreateInsertValue(slice_value, ptr_value, { 0 }, "slice.data");
    return compiler.get_builder().CreateInsertValue(slice_value, size_value, { 1 }, "slice.size");
}

llvm::Type* SliceNode::gen_type() const
{
    return ASTNode::get_llvm_type(get_ktype(), compiler);
}

KType* SliceNode::get_ktype() const
{
    validate();
    if (!cached_type) {
        cached_type = new SliceType(ptr->get_ktype()->as<PointerType>()->get_pointee()->copy());
    }
    return cached_type;
}

llvm::Value* SliceNode::trivial_gen()
{
    return nullptr;
}

bool SliceNode::is_trivially_evaluable() const
{
    return false;
}

std::vector<ASTNode*> SliceNode::get_children() const
{
    return { ptr, size };
}

void SliceNode::validate() const
{
    auto* ptr_type = ptr->get_ktype();
    if (!ptr_type->is_pointer()) {
        throw std::runtime_error(
            std::format("Slice data expression must be a pointer, got `{}`", ptr_type->to_string()));
    }

    auto* size_type = size->get_ktype();
    if (!size_type->is_integer()) {
        throw std::runtime_error(
            std::format("Slice size expression must be an integer, got `{}`", size_type->to_string()));
    }
}
