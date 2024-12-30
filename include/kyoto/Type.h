#include <string>

class Type {
public:
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
};

class PrimitiveType : public Type {
public:
    enum class Kind { Boolean, Char, I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, String, Void, Unknown };

public:
    explicit PrimitiveType(Kind kind);
    std::string to_string() const override;

    bool is_integer() const;
    bool is_signed() const;
    bool is_unsigned() const;
    bool is_floating_point() const;
    bool is_boolean() const;
    bool is_numeric() const;
    bool is_char() const;

private:
    PrimitiveTypeKind kind;
};