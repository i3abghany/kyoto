#include "kyoto/KType.h"

#include <assert.h>

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
    case Kind::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

bool PrimitiveType::operator==(const KType& other) const
{
    auto* other_primitive = dynamic_cast<const PrimitiveType*>(&other);
    if (!other_primitive) return false;

    return kind == other_primitive->kind;
}

bool PrimitiveType::is_integer() const
{
    return kind == Kind::I8 || kind == Kind::I16 || kind == Kind::I32 || kind == Kind::I64;
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

    assert(false && "Unknown type");
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

bool PointerType::is_string() const
{
    auto* pt = dynamic_cast<PrimitiveType*>(pointee);
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

size_t PointerType::ptr_level() const
{
    return 1 + pointee->ptr_level();
}