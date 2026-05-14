#include "kyoto/AST/Expressions/ArrayIndexNode.h"

#include <format>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <stdexcept>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"

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
    } else if (array_ktype->is_slice()) {
        return gen_slice_access();
    } else {
        throw std::runtime_error(std::format("Cannot index into type '{}'", array_ktype->to_string()));
    }
}

llvm::Value* ArrayIndexNode::gen_ptr() const
{
    const_cast<ArrayIndexNode*>(this)->validate_index_type();

    auto* array_ktype = array->get_ktype();

    if (array_ktype->is_array()) {
        auto* index_val = index->gen();
        auto* array_ptr = array->gen_ptr();
        if (!array_ptr) {
            throw std::runtime_error("Cannot get pointer to array");
        }

        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(compiler.get_context()), 0);
        return compiler.get_builder().CreateGEP(array->gen_type(), array_ptr, { zero, index_val }, "arrayptr");
    } else if (array_ktype->is_pointer()) {
        auto* index_val = index->gen();
        auto* array_val = array->gen();
        return compiler.get_builder().CreateGEP(gen_type(), array_val, index_val, "arrayptr");
    } else if (array_ktype->is_slice()) {
        return gen_slice_element_ptr();
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

llvm::Value* ArrayIndexNode::gen_slice_access() const
{
    auto* element_ptr = gen_ptr();
    return compiler.get_builder().CreateLoad(gen_type(), element_ptr, "arrayload");
}

llvm::Value* ArrayIndexNode::gen_slice_element_ptr() const
{
    auto* slice_value = array->gen();
    auto* data_ptr = compiler.get_builder().CreateExtractValue(slice_value, { 0 }, "slice.data");
    auto* size_value = compiler.get_builder().CreateExtractValue(slice_value, { 1 }, "slice.size");
    auto* raw_index = index->gen();
    auto* i64_type = llvm::Type::getInt64Ty(compiler.get_context());
    auto* index_value = raw_index->getType()->isIntegerTy(64)
        ? raw_index
        : compiler.get_builder().CreateIntCast(raw_index, i64_type, true, "slice.index");

    auto* zero = llvm::ConstantInt::get(i64_type, 0, true);
    auto* non_negative = compiler.get_builder().CreateICmpSGE(index_value, zero, "slice.index.nonnegative");
    auto* below_size = compiler.get_builder().CreateICmpSLT(index_value, size_value, "slice.index.inrange");
    auto* in_bounds = compiler.get_builder().CreateAnd(non_negative, below_size, "slice.index.ok");

    auto* fn = compiler.get_builder().GetInsertBlock()->getParent();
    auto* trap_bb = llvm::BasicBlock::Create(compiler.get_context(), "slice_oob", fn);
    auto* ok_bb = llvm::BasicBlock::Create(compiler.get_context(), "slice_inbounds", fn);

    compiler.get_builder().CreateCondBr(in_bounds, ok_bb, trap_bb);

    compiler.get_builder().SetInsertPoint(trap_bb);
    auto* puts_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(compiler.get_context()),
                                              { llvm::PointerType::get(compiler.get_context(), 0) }, false);
    auto puts_fn = compiler.get_module()->getOrInsertFunction("puts", puts_type);
    auto* message = compiler.get_builder().CreateGlobalString("runtime error: slice index out of bounds");
    compiler.get_builder().CreateCall(puts_fn, { message });
    auto* ptr_type = llvm::PointerType::get(compiler.get_context(), 0);
    auto* fflush_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(compiler.get_context()), { ptr_type }, false);
    auto fflush_fn = compiler.get_module()->getOrInsertFunction("fflush", fflush_type);
    compiler.get_builder().CreateCall(fflush_fn, { llvm::ConstantPointerNull::get(ptr_type) });
    auto trap_fn = llvm::Intrinsic::getOrInsertDeclaration(compiler.get_module(), llvm::Intrinsic::trap);
    compiler.get_builder().CreateCall(trap_fn);
    compiler.get_builder().CreateUnreachable();

    compiler.get_builder().SetInsertPoint(ok_bb);
    return compiler.get_builder().CreateGEP(gen_type(), data_ptr, index_value, "sliceptr");
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
    } else if (array_ktype->is_slice()) {
        return array_ktype->as<SliceType>()->get_element_type()->copy();
    } else {
        throw std::runtime_error(std::format("Cannot index into type '{}'", array_ktype->to_string()));
    }
}
