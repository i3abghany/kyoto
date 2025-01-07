#pragma once

#include <stddef.h>
#include <string>

namespace llvm {
class Type;
}

class KType {
public:
    virtual ~KType() = default;
    virtual std::string to_string() const = 0;
};

class PrimitiveType : public KType {
public:
    enum class Kind { Boolean, Char, I8, I16, I32, I64, F32, F64, String, Void, Unknown };

public:
    explicit PrimitiveType(Kind kind);
    std::string to_string() const override;

    bool is_integer() const;
    bool is_floating_point() const;
    bool is_boolean() const;
    bool is_numeric() const;
    bool is_char() const;
    bool is_string() const;
    bool is_void() const;

    size_t width() const;

    Kind get_kind() const;

    static PrimitiveType from_llvm_type(const llvm::Type* type);

private:
    PrimitiveType::Kind kind;
};