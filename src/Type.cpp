#include "kyoto/Type.h"

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
    case Kind::U8:
        return "U8";
    case Kind::U16:
        return "U16";
    case Kind::U32:
        return "U32";
    case Kind::U64:
        return "U64";
    case Kind::F32:
        return "F32";
    case Kind::F64:
        return "F64";
    case Kind::String:
        return "String";
    case Kind::Void:
        return "Void";
    }
    return "Unknown";
}

bool PrimitiveType::is_integer() const
{
    return is_signed() || is_unsigned() || is_boolean() || is_char();
}

bool PrimitiveType::is_signed() const
{
    return kind == Kind::I8 || kind == Kind::I16 || kind == Kind::I32 || kind == Kind::I64;
}

bool PrimitiveType::is_unsigned() const
{
    return kind == Kind::U8 || kind == Kind::U16 || kind == Kind::U32 || kind == Kind::U64;
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