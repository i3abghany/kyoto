#pragma once

#include <fmt/core.h>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace llvm {
class Type;
}

class KType {
public:
    virtual ~KType() = default;
    virtual std::string to_string() const = 0;
    virtual bool is_primitive() const { return false; }
    virtual bool is_pointer() const { return false; }
    virtual bool is_class() const { return false; }
    virtual bool is_void() const { return false; }
    virtual bool is_string() const { return false; }
    virtual bool is_integer() const { return false; }
    virtual bool is_floating_point() const { return false; }
    virtual bool is_boolean() const { return false; }
    virtual bool is_numeric() const { return false; }
    virtual bool is_char() const { return false; }

    virtual KType* copy() const = 0;
    virtual bool operator==(const KType& other) const = 0;

    static KType* get_void();

    template <typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

    template <typename T> T* as()
    {
        auto* ptr = dynamic_cast<T*>(this);
        if (!ptr) {
            throw std::runtime_error(fmt::format("KType::as: Cannot cast {} to {}", to_string(), typeid(T).name()));
        }
        return ptr;
    }

    template <typename T> const T* as() const
    {
        auto* ptr = dynamic_cast<const T*>(this);
        if (!ptr) {
            throw std::runtime_error(fmt::format("KType::as: Cannot cast {} to {}", to_string(), typeid(T).name()));
        }
        return ptr;
    }

    static KType* from_llvm_type(const llvm::Type* type);
};

class PrimitiveType : public KType {
public:
    enum class Kind { Boolean, Char, I8, I16, I32, I64, F32, F64, String, Void, Unknown };

    explicit PrimitiveType(Kind kind);
    std::string to_string() const override;
    [[nodiscard]] bool is_primitive() const override;
    bool operator==(const KType& other) const override;
    KType* copy() const override;

    bool is_integer() const override;
    bool is_floating_point() const override;
    bool is_boolean() const override;
    bool is_numeric() const override;
    bool is_char() const override;
    bool is_void() const override;

    size_t width() const;

    Kind get_kind() const;

private:
    Kind kind;
};

class PointerType : public KType {
public:
    explicit PointerType(KType* pointee);
    ~PointerType() override;
    std::string to_string() const override;
    bool operator==(const KType& other) const override;
    KType* copy() const override;

    KType* get_pointee() const;
    bool is_string() const override;
    bool is_pointer() const override;

private:
    KType* pointee;
};

class ClassType : public KType {
public:
    explicit ClassType(std::string name);
    std::string to_string() const override;
    [[nodiscard]] std::string get_name() const;
    [[nodiscard]] bool is_class() const override;
    bool operator==(const KType& other) const override;
    KType* copy() const override;

private:
    std::string name;
};