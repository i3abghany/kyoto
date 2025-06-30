#include "kyoto/KType.h"

#include <assert.h>
#include <utility>

#include "llvm/IR/Type.h"

KType* KType::get_void()
{
    static PrimitiveType void_type(PrimitiveType::Kind::Void);
    return &void_type;
}

PrimitiveType::PrimitiveType(Kind kind)
    : kind(kind)
{
}

std::string PrimitiveType::to_string() const
{
    switch (kind) {
    case Kind::Boolean:
        return "Boolean";
    case Kind::Char:
        return "Char";
    case Kind::I8:
        return "I8";
    case Kind::I16:
        return "I16";
    case Kind::I32:
        return "I32";
    case Kind::I64:
        return "I64";
    case Kind::F32:
        return "F32";
    case Kind::F64:
        return "F64";
    case Kind::Void:
        return "Void";
    default:
        return "Unknown";
    }
}

bool PrimitiveType::is_primitive() const
{
    return true;
}

bool PrimitiveType::operator==(const KType& other) const
{
    auto* other_primitive = dynamic_cast<const PrimitiveType*>(&other);
    if (!other_primitive) return false;

    return kind == other_primitive->kind;
}

bool PrimitiveType::is_integer() const
{
    return kind == Kind::I8 || kind == Kind::I16 || kind == Kind::I32 || kind == Kind::I64 || is_char();
}

bool PrimitiveType::is_floating_point() const
{
    return kind == Kind::F32 || kind == Kind::F64;
}

bool PrimitiveType::is_boolean() const
{
    return kind == Kind::Boolean;
}

bool PrimitiveType::is_numeric() const
{
    return is_integer() || is_floating_point();
}

bool PrimitiveType::is_char() const
{
    return kind == Kind::Char;
}

bool PrimitiveType::is_void() const
{
    return kind == Kind::Void;
}

size_t PrimitiveType::width() const
{
    switch (kind) {
    case Kind::Boolean:
    case Kind::Char:
    case Kind::I8:
        return 1;

    case Kind::I16:
        return 2;

    case Kind::I32:
    case Kind::F32:
        return 4;

    case Kind::I64:
    case Kind::F64:
        return 8;

    default:
        break;
    }

    throw std::runtime_error("Unknown primitive type width");
}

KType* PrimitiveType::copy() const
{
    return new PrimitiveType(kind);
}

PrimitiveType::Kind PrimitiveType::get_kind() const
{
    return kind;
}

KType* KType::from_llvm_type(const llvm::Type* type)
{
    using Kind = PrimitiveType::Kind;
    if (type->isIntegerTy(1)) return new PrimitiveType(Kind::Boolean);
    if (type->isIntegerTy(8)) return new PrimitiveType(Kind::I8);
    if (type->isIntegerTy(16)) return new PrimitiveType(Kind::I16);
    if (type->isIntegerTy(32)) return new PrimitiveType(Kind::I32);
    if (type->isIntegerTy(64)) return new PrimitiveType(Kind::I64);

    if (type->isFloatTy()) return new PrimitiveType(Kind::F32);
    if (type->isDoubleTy()) return new PrimitiveType(Kind::F64);

    // The only "pointer" type we support for now is string, so we can safely
    // return it here.
    if (type->isPointerTy()) return new PointerType(new PrimitiveType(Kind::Char));

    if (type->isVoidTy()) return new PrimitiveType(Kind::Void);

    return new PrimitiveType(Kind::Unknown);
}

PointerType::PointerType(KType* pointee)
    : pointee(pointee)
{
}

PointerType::~PointerType()
{
    delete pointee;
}

std::string PointerType::to_string() const
{
    return pointee->to_string() + "*";
}

bool PointerType::is_pointer() const
{
    return true;
}

KType* PointerType::get_pointee() const
{
    return pointee;
}

bool PointerType::is_pointer_to_class(const std::string& name) const
{
    if (name.empty()) return pointee->is_class();
    return pointee->is_class() && pointee->get_class_name() == name;
}

std::string PointerType::get_class_name() const
{
    return pointee->as<ClassType>()->get_class_name();
}

bool PointerType::is_string() const
{
    const auto* pt = dynamic_cast<PrimitiveType*>(pointee);
    return pt && pt->get_kind() == PrimitiveType::Kind::Char;
}

bool PointerType::operator==(const KType& other) const
{
    auto* other_pointer = dynamic_cast<const PointerType*>(&other);
    if (!other_pointer) return false;

    return *pointee == *other_pointer->pointee;
}

KType* PointerType::copy() const
{
    return new PointerType(pointee->copy());
}

ClassType::ClassType(std::string name)
    : name(std::move(name))
{
}

ClassType::~ClassType() = default;

std::string ClassType::to_string() const
{
    return "Class " + name;
}

bool ClassType::is_class() const
{
    return true;
}

bool ClassType::operator==(const KType& other) const
{
    auto* other_class = dynamic_cast<const ClassType*>(&other);
    if (!other_class) return false;

    return name == other_class->name;
}

KType* ClassType::copy() const
{
    return new ClassType(name);
}

std::string ClassType::get_class_name() const
{
    return name;
}

ArrayType::ArrayType(KType* element_type, size_t n)
    : element_type(element_type)
    , size(n)
{
}

ArrayType::~ArrayType()
{
    delete element_type;
}

std::string ArrayType::to_string() const
{
    return std::format("{}[{}]", element_type->to_string(), size);
}

bool ArrayType::operator==(const KType& other) const
{
    auto* other_array = dynamic_cast<const ArrayType*>(&other);
    if (!other_array) return false;

    return *element_type == *other_array->element_type && size == other_array->size;
}

KType* ArrayType::copy() const
{
    return new ArrayType(element_type->copy());
}

KType* ArrayType::get_element_type() const
{
    return element_type;
}

bool ArrayType::is_array() const
{
    return true;
}

size_t ArrayType::get_size() const
{
    return size;
}

void ArrayType::set_size(size_t n)
{
    size = n;
}