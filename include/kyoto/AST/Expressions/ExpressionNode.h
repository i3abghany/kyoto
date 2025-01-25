#pragma once

#include <stdint.h>
#include <string>

#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;
class KType;
class PrimitiveType;

namespace llvm {
class Type;
class Value;
}

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
    [[nodiscard]] virtual std::string to_string() const = 0;
    [[nodiscard]] virtual llvm::Value* gen() = 0;
    [[nodiscard]] virtual llvm::Type* gen_type() const = 0;
    [[nodiscard]] virtual KType* get_ktype() const { return nullptr; }
    [[nodiscard]] virtual llvm::Value* trivial_gen() { return nullptr; }
    [[nodiscard]] virtual bool is_trivially_evaluable() const { return false; }
    [[nodiscard]] virtual llvm::Value* gen_ptr() const { return nullptr; }

    [[nodiscard]] virtual std::vector<ASTNode*> get_children() const = 0;

    static void check_boolean_promotion(PrimitiveType* expr_ktype, PrimitiveType* target_type,
                                        const std::string& target_name);

    static void check_int_range_fit(int64_t val, PrimitiveType* target_type, ModuleCompiler& compiler,
                                    const std::string& expr, const std::string& target_name);

    static llvm::Value* dynamic_integer_conversion(llvm::Value* expr_val, PrimitiveType* expr_ktype,
                                                   PrimitiveType* target_type, ModuleCompiler& compiler);

    static llvm::Value* promoted_trivially_gen(ExpressionNode* expr, ModuleCompiler& compiler, KType* target_ktype,
                                               const std::string& target_name);

    static llvm::Value* handle_integer_conversion(ExpressionNode* expr, KType* target_type, ModuleCompiler& compiler,
                                                  const std::string& what, const std::string& target_name = "");
};
