#include "kyoto/AST/Expressions/SizeofNode.h"

#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"

SizeofNode::SizeofNode(ExpressionNode* expr, ModuleCompiler& compiler)
    : operand_type(OperandType::Expression)
    , expr(expr)
    , type(nullptr)
    , compiler(compiler)
{
}

SizeofNode::SizeofNode(KType* type, ModuleCompiler& compiler)
    : operand_type(OperandType::Type)
    , expr(nullptr)
    , type(type)
    , compiler(compiler)
{
}

SizeofNode::~SizeofNode()
{
    if (operand_type == OperandType::Expression && expr) {
        delete expr;
    }
    if (operand_type == OperandType::Type && type) {
        delete type;
    }
}

std::string SizeofNode::to_string() const
{
    if (operand_type == OperandType::Expression) {
        return std::format("sizeof({})", expr->to_string());
    } else {
        return std::format("sizeof({})", type->to_string());
    }
}

llvm::Value* SizeofNode::gen()
{
    KType* target_type;

    if (operand_type == OperandType::Expression) {
        target_type = expr->get_ktype();
    } else {
        target_type = type;
    }

    size_t size_bytes = 0;

    if (target_type->is_primitive()) {
        auto* prim_type = target_type->as<PrimitiveType>();
        size_bytes = prim_type->width();
    } else if (target_type->is_pointer()) {
        // TODO: infer pointer size from context
        size_bytes = 8;
    } else if (target_type->is_array()) {
        auto* array_type = target_type->as<ArrayType>();
        KType* element_type = array_type->get_element_type();

        size_t element_size;
        if (element_type->is_primitive()) {
            element_size = element_type->as<PrimitiveType>()->width();
        } else if (element_type->is_pointer()) {
            element_size = 8;
        } else if (element_type->is_class()) {
            element_size = compiler.get_type_size(element_type->get_class_name());
        } else {
            throw std::runtime_error("Unsupported element type in sizeof array");
        }

        size_bytes = element_size * array_type->get_size();
    } else if (target_type->is_class()) {
        size_bytes = compiler.get_type_size(target_type->get_class_name());
    } else {
        throw std::runtime_error("Unsupported type in sizeof expression");
    }

    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(compiler.get_context()), size_bytes);
}

llvm::Value* SizeofNode::gen_ptr() const
{
    throw std::runtime_error("Cannot take address of sizeof expression");
}

llvm::Type* SizeofNode::gen_type() const
{
    return llvm::Type::getInt32Ty(compiler.get_context());
}

KType* SizeofNode::get_ktype() const
{
    return new PrimitiveType(PrimitiveType::Kind::I32);
}

llvm::Value* SizeofNode::trivial_gen()
{
    return gen();
}

bool SizeofNode::is_trivially_evaluable() const
{
    return true;
}

std::vector<ASTNode*> SizeofNode::get_children() const
{
    if (operand_type == OperandType::Expression && expr) {
        return { expr };
    }
    return {};
}
