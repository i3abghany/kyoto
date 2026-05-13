#include "kyoto/AST/Expressions/FunctionCallNode.h"

#include <assert.h>
#include <format>
#include <optional>
#include <stddef.h>
#include <stdexcept>
#include <utility>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

namespace {

enum class ArgumentMatch {
    None,
    Conversion,
    Exact,
};

std::string format_argument_types(const std::vector<ExpressionNode*>& args)
{
    std::string result;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) result += ", ";
        result += args[i]->get_ktype()->to_string();
    }
    return result;
}

llvm::FunctionType* build_llvm_function_type(const FunctionType* type, ModuleCompiler& compiler)
{
    std::vector<llvm::Type*> arg_types;
    arg_types.reserve(type->get_param_types().size());
    for (auto* arg_type : type->get_param_types())
        arg_types.push_back(ASTNode::get_llvm_type(arg_type, compiler));
    return llvm::FunctionType::get(ASTNode::get_llvm_type(type->get_return_type(), compiler), arg_types, false);
}

ArgumentMatch match_argument(ExpressionNode* arg, const KType* param_type, ModuleCompiler& compiler)
{
    const auto* arg_type = arg->get_ktype();
    if (*param_type == *arg_type) return ArgumentMatch::Exact;

    if (!((param_type->is_integer() && arg_type->is_integer())
            || (param_type->is_boolean() && arg_type->is_boolean()))) {
        return ArgumentMatch::None;
    }

    const auto* primitive_param = param_type->as<PrimitiveType>();
    const auto* primitive_arg = arg_type->as<PrimitiveType>();
    if (compiler.get_type_resolver().promotable_to(primitive_arg->get_kind(), primitive_param->get_kind())) {
        return ArgumentMatch::Conversion;
    }

    return arg->is_trivially_evaluable() ? ArgumentMatch::Conversion : ArgumentMatch::None;
}

ArgumentMatch match_candidate(FunctionNode* candidate, const std::vector<ExpressionNode*>& args, size_t param_offset,
                              ModuleCompiler& compiler)
{
    const auto& params = candidate->get_params();
    const auto total_arg_count = args.size() + param_offset;
    if (candidate->is_varargs()) {
        if (params.size() > total_arg_count) return ArgumentMatch::None;
    } else if (params.size() != total_arg_count) {
        return ArgumentMatch::None;
    }

    auto result = ArgumentMatch::Exact;
    for (size_t i = 0; i + param_offset < params.size(); ++i) {
        const auto arg_match = match_argument(args[i], params[param_offset + i].type, compiler);
        if (arg_match == ArgumentMatch::None) return ArgumentMatch::None;
        if (arg_match == ArgumentMatch::Conversion) result = ArgumentMatch::Conversion;
    }

    return result;
}

std::optional<FunctionNode*> select_overload(const std::string& name, const std::vector<ExpressionNode*>& args,
                                             ModuleCompiler& compiler, size_t total_arity, size_t param_offset)
{
    auto candidates = compiler.get_functions(name, total_arity);
    std::vector<FunctionNode*> exact_matches;
    std::vector<FunctionNode*> conversion_matches;

    for (auto* candidate : candidates) {
        switch (match_candidate(candidate, args, param_offset, compiler)) {
        case ArgumentMatch::Exact:
            exact_matches.push_back(candidate);
            break;
        case ArgumentMatch::Conversion:
            conversion_matches.push_back(candidate);
            break;
        case ArgumentMatch::None:
            break;
        }
    }

    if (exact_matches.size() == 1) return exact_matches.front();
    if (exact_matches.size() > 1) {
        throw std::runtime_error(std::format("Call to function `{}` with argument types ({}) is ambiguous", name,
                                             format_argument_types(args)));
    }

    if (conversion_matches.size() == 1) return conversion_matches.front();
    if (conversion_matches.size() > 1) {
        throw std::runtime_error(std::format("Call to function `{}` with argument types ({}) is ambiguous", name,
                                             format_argument_types(args)));
    }

    return std::nullopt;
}

std::optional<FunctionNode*> select_declared_function(const std::string& name, const std::vector<ExpressionNode*>& args,
                                                      ModuleCompiler& compiler, size_t total_arity,
                                                      size_t param_offset)
{
    if (param_offset == 0) {
        if (auto fn = compiler.get_external_varargs_function(name, total_arity); fn.has_value()) return fn;
    }
    return select_overload(name, args, compiler, total_arity, param_offset);
}

std::vector<llvm::Value*> build_call_arg_values(FunctionNode* fn_meta, const std::string& name,
                                                const std::vector<ExpressionNode*>& args, ModuleCompiler& compiler,
                                                llvm::Value* destination = nullptr)
{
    std::vector<llvm::Value*> arg_values;
    size_t param_offset = 0;

    if (destination) {
        arg_values.push_back(destination);
        param_offset = 1;
    }

    const auto& params = fn_meta->get_params();
    for (size_t i = 0; i < args.size(); ++i) {
        auto* arg = args[i];
        const auto param_index = param_offset + i;

        if (param_index >= params.size()) {
            arg_values.push_back(arg->gen());
            continue;
        }

        const auto* param_type = params[param_index].type;
        const auto* arg_type = arg->get_ktype();

        if ((param_type->is_integer() && arg_type->is_integer())
            || (param_type->is_boolean() && arg_type->is_boolean())) {
            arg_values.push_back(ExpressionNode::handle_integer_conversion(
                arg, param_type, compiler, "pass as argument", name + "::" + params[param_index].name));
            continue;
        }

        if ((param_type->is_pointer() && arg_type->is_pointer() && *param_type == *arg_type)
            || (param_type->is_string() && arg_type->is_string())
            || (param_type->is_array() && arg_type->is_array() && *param_type == *arg_type)
            || (param_type->is_class() && arg_type->is_class() && *param_type == *arg_type)
            || (param_type->is_function() && arg_type->is_function() && *param_type == *arg_type)) {
            arg_values.push_back(arg->gen());
            continue;
        }

        throw std::runtime_error(std::format(
            "Cannot pass argument `{}` (type `{}`) to parameter `{}` of function `{}` (expected `{}`)",
            arg->to_string(), arg_type->to_string(), params[param_index].name, name, param_type->to_string()));
    }

    return arg_values;
}

std::vector<llvm::Value*> build_call_arg_values(const FunctionType* function_type,
                                                const std::vector<ExpressionNode*>& args, ModuleCompiler& compiler,
                                                const std::string& callee_name)
{
    const auto& params = function_type->get_param_types();
    if (params.size() != args.size()) {
        throw std::runtime_error(
            std::format("Function `{}` expects {} arguments, got {}", callee_name, params.size(), args.size()));
    }

    std::vector<llvm::Value*> arg_values;
    arg_values.reserve(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        auto* arg = args[i];
        const auto* param_type = params[i];
        const auto* arg_type = arg->get_ktype();

        if ((param_type->is_integer() && arg_type->is_integer())
            || (param_type->is_boolean() && arg_type->is_boolean())) {
            arg_values.push_back(
                ExpressionNode::handle_integer_conversion(arg, param_type, compiler, "pass as argument", callee_name));
            continue;
        }

        if ((param_type->is_pointer() && arg_type->is_pointer() && *param_type == *arg_type)
            || (param_type->is_string() && arg_type->is_string())
            || (param_type->is_array() && arg_type->is_array() && *param_type == *arg_type)
            || (param_type->is_class() && arg_type->is_class() && *param_type == *arg_type)
            || (param_type->is_function() && arg_type->is_function() && *param_type == *arg_type)) {
            arg_values.push_back(arg->gen());
            continue;
        }

        throw std::runtime_error(std::format("Cannot pass argument `{}` (type `{}`) to function `{}` (expected `{}`)",
                                             arg->to_string(), arg_type->to_string(), callee_name,
                                             param_type->to_string()));
    }

    return arg_values;
}

} // namespace

FunctionCall::FunctionCall(std::string name, std::vector<ExpressionNode*> args, ModuleCompiler& compiler)
    : name(std::move(name))
    , args(std::move(args))
    , compiler(compiler)
{
}

FunctionCall::FunctionCall(ExpressionNode* callee, std::vector<ExpressionNode*> args, ModuleCompiler& compiler)
    : callee(callee)
    , args(std::move(args))
    , compiler(compiler)
{
}

FunctionCall::~FunctionCall()
{
    delete callee;
    for (const auto arg : args)
        delete arg;
}

std::string FunctionCall::to_string() const
{
    std::string str = (callee ? callee->to_string() : name) + "(";
    for (size_t i = 0; i < args.size(); i++) {
        str += args[i]->to_string();
        if (i != args.size() - 1) {
            str += ", ";
        }
    }
    str += ")";
    return str;
}

namespace {

const FunctionType* get_symbol_function_type(const std::string& name, ModuleCompiler& compiler)
{
    auto symbol = compiler.get_symbol(name);
    if (symbol.has_value()) return dynamic_cast<const FunctionType*>(symbol->type);

    if (const auto pos = name.rfind("__"); pos != std::string::npos) {
        symbol = compiler.get_symbol(name.substr(pos + 2));
        if (symbol.has_value()) return dynamic_cast<const FunctionType*>(symbol->type);
    }

    return nullptr;
}

llvm::Value* build_symbol_function_callee(const std::string& name, ModuleCompiler& compiler)
{
    auto symbol = compiler.get_symbol(name);
    if (!symbol.has_value() && name.rfind("__") != std::string::npos) {
        symbol = compiler.get_symbol(name.substr(name.rfind("__") + 2));
    }
    if (!symbol.has_value()) throw std::runtime_error(std::format("Unknown symbol `{}`", name));
    return compiler.get_builder().CreateLoad(symbol->alloc->getAllocatedType(), symbol->alloc, name);
}

}

llvm::Value* FunctionCall::gen()
{
    if (callee) {
        const auto* function_type = dynamic_cast<const FunctionType*>(callee->get_ktype());
        if (!function_type) {
            throw std::runtime_error(std::format("Cannot call non-function expression `{}` of type `{}`",
                                                 callee->to_string(), callee->get_ktype()->to_string()));
        }

        const auto arg_values = build_call_arg_values(function_type, args, compiler, callee->to_string());
        return compiler.get_builder().CreateCall(build_llvm_function_type(function_type, compiler), callee->gen(),
                                                 arg_values);
    }

    if (const auto* function_type = get_symbol_function_type(name, compiler)) {
        const auto arg_values = build_call_arg_values(function_type, args, compiler, name);
        return compiler.get_builder().CreateCall(build_llvm_function_type(function_type, compiler),
                                                 build_symbol_function_callee(name, compiler), arg_values);
    }

    if (name == "main") {
        auto* fn = compiler.get_module()->getFunction("main");
        if (fn) {
            std::vector<llvm::Value*> arg_values;
            for (auto& arg : args)
                arg_values.push_back(arg->gen());
            return compiler.get_builder().CreateCall(fn, arg_values);
        }
    }

    if (is_constructor_call() && !destination) {
        throw std::runtime_error(std::format("Constructor call `{}` needs a destination object", name));
    }

    const size_t param_offset = is_constructor_call() ? 1 : 0;
    const size_t lookup_arity = args.size() + param_offset;
    auto fn_meta = select_declared_function(name, args, compiler, lookup_arity, param_offset);
    const auto llvm_name = fn_meta.has_value() ? compiler.get_function_llvm_name(fn_meta.value())
                                               : name + "_" + std::to_string(lookup_arity);

    auto* fn = compiler.get_module()->getFunction(llvm_name);

    if (!fn && fn_meta.has_value() && fn_meta.value()->get_linkage_name() == "main") {
        fn = compiler.get_module()->getFunction("main");
    }

    if (!fn) {
        throw std::runtime_error(
            std::format("Function `{}` with {} arguments not found (looking for `{}`)", name, args.size(), llvm_name));
    }

    std::vector<llvm::Value*> arg_values;
    if (destination && is_constructor_call()) {
        arg_values = build_call_arg_values(fn_meta.value(), name, args, compiler, destination);
    } else {
        arg_values = build_call_arg_values(fn_meta.value(), name, args, compiler);
    }

    if (fn->arg_size() != arg_values.size() && (!fn->isVarArg() || fn->arg_size() > arg_values.size())) {
        throw std::runtime_error(
            std::format("Function `{}` expects {} arguments, got {}", name, fn->arg_size(), arg_values.size()));
    }
    return compiler.get_builder().CreateCall(fn, arg_values);
}

llvm::Value* FunctionCall::gen_ptr() const
{
    assert(get_ktype()->is_pointer() && "Function does not return a pointer");
    if (callee) {
        const auto* function_type = dynamic_cast<const FunctionType*>(callee->get_ktype());
        if (!function_type) {
            throw std::runtime_error(std::format("Cannot call non-function expression `{}` of type `{}`",
                                                 callee->to_string(), callee->get_ktype()->to_string()));
        }

        const auto arg_values = build_call_arg_values(function_type, args, compiler, callee->to_string());
        return compiler.get_builder().CreateCall(build_llvm_function_type(function_type, compiler), callee->gen(),
                                                 arg_values);
    }

    if (const auto* function_type = get_symbol_function_type(name, compiler)) {
        const auto arg_values = build_call_arg_values(function_type, args, compiler, name);
        return compiler.get_builder().CreateCall(build_llvm_function_type(function_type, compiler),
                                                 build_symbol_function_callee(name, compiler), arg_values);
    }

    if (is_constructor_call() && !destination) {
        throw std::runtime_error(std::format("Constructor call `{}` needs a destination object", name));
    }

    const size_t param_offset = is_constructor_call() ? 1 : 0;
    const size_t lookup_arity = args.size() + param_offset;
    auto fn_meta = select_declared_function(name, args, compiler, lookup_arity, param_offset);
    const auto llvm_name = fn_meta.has_value() ? compiler.get_function_llvm_name(fn_meta.value())
                                               : name + "_" + std::to_string(lookup_arity);

    std::vector<llvm::Value*> arg_values;
    if (destination && is_constructor_call()) {
        arg_values = build_call_arg_values(fn_meta.value(), name, args, compiler, destination);
    } else {
        arg_values = build_call_arg_values(fn_meta.value(), name, args, compiler);
    }

    auto* fn = compiler.get_module()->getFunction(llvm_name);

    if (!fn) {
        throw std::runtime_error(std::format("Function `{}` with {} arguments not found", name, args.size()));
    }
    return compiler.get_builder().CreateCall(fn, arg_values);
}

llvm::Type* FunctionCall::gen_type() const
{
    if (callee) {
        const auto* function_type = dynamic_cast<const FunctionType*>(callee->get_ktype());
        if (!function_type) {
            throw std::runtime_error(std::format("Cannot call non-function expression `{}` of type `{}`",
                                                 callee->to_string(), callee->get_ktype()->to_string()));
        }
        return ASTNode::get_llvm_type(function_type->get_return_type(), compiler);
    }

    if (const auto* function_type = get_symbol_function_type(name, compiler)) {
        return ASTNode::get_llvm_type(function_type->get_return_type(), compiler);
    }

    const size_t param_offset = is_constructor_call() ? 1 : 0;
    const size_t lookup_arity = args.size() + param_offset;
    auto fn_meta = select_declared_function(name, args, compiler, lookup_arity, param_offset);
    const auto llvm_name = fn_meta.has_value() ? compiler.get_function_llvm_name(fn_meta.value())
                                               : name + "_" + std::to_string(lookup_arity);

    const auto* fn = compiler.get_module()->getFunction(llvm_name);

    if (!fn) {
        throw std::runtime_error(std::format("Function `{}` with {} arguments not found", name, args.size()));
    }

    return fn->getReturnType();
}

llvm::Value* FunctionCall::trivial_gen()
{
    assert(false && "Function call is not trivially evaluable");
}

KType* FunctionCall::get_ktype() const
{
    if (callee) {
        const auto* function_type = dynamic_cast<const FunctionType*>(callee->get_ktype());
        if (!function_type) {
            throw std::runtime_error(std::format("Cannot call non-function expression `{}` of type `{}`",
                                                 callee->to_string(), callee->get_ktype()->to_string()));
        }
        return function_type->get_return_type();
    }

    if (const auto* function_type = get_symbol_function_type(name, compiler)) {
        return const_cast<FunctionType*>(function_type)->get_return_type();
    }

    const size_t param_offset = is_constructor_call() ? 1 : 0;
    const size_t lookup_arity = args.size() + param_offset;
    auto fn = select_declared_function(name, args, compiler, lookup_arity, param_offset);
    if (fn.has_value()) return fn.value()->get_ret_type();

    throw std::runtime_error(std::format("Function `{}` with {} arguments not found", name, args.size()));
}

bool FunctionCall::is_trivially_evaluable() const
{
    return false;
}

void FunctionCall::set_destination(llvm::Value* dest)
{
    if (destination) throw std::runtime_error("Destination already set");
    destination = dest;
}

void FunctionCall::insert_arg(ExpressionNode* node, size_t index)
{
    if (index == 0 && destination)
        throw std::runtime_error("Cannot insert argument at index 0 when destination is set");
    args.insert(args.begin() + index, node);
}

std::vector<ASTNode*> FunctionCall::get_children() const
{
    std::vector<ASTNode*> children;
    if (callee) children.push_back(callee);
    children.insert(children.end(), args.begin(), args.end());
    return children;
}
