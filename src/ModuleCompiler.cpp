#include "kyoto/ModuleCompiler.h"

#include <BailErrorStrategy.h>
#include <BaseErrorListener.h>
#include <Exceptions.h>
#include <Parser.h>
#include <RecognitionException.h>
#include <Token.h>
#include <Vocabulary.h>
#include <algorithm>
#include <any>
#include <assert.h>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TypeSize.h>
#include <misc/IntervalSet.h>
#include <regex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "KyotoLexer.h"
#include "KyotoParser.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/Analysis/FunctionTermination.h"
#include "kyoto/KType.h"
#include "kyoto/Resolution/ClassIdentifierVisitor.h"
#include "kyoto/Resolution/ConstructorIdentifierVisitor.h"
#include "kyoto/Resolution/FunctionIdentifierVisitor.h"
#include "kyoto/Visitor.h"
#include "kyoto/utils/File.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

class LexerErrorListener : public antlr4::BaseErrorListener {
public:
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                     size_t charPositionInLine, const std::string& msg, std::exception_ptr e) override
    {
        throw std::runtime_error(std::format("Lexer error at line {}, char {}: {}", line, charPositionInLine, msg));
    }
};

class ParserErrorListener : public antlr4::BaseErrorListener {
public:
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                     size_t charPositionInLine, const std::string& msg, std::exception_ptr e) override
    {
        throw std::runtime_error(std::format("Parser error at line {}, char {}: {}", line, charPositionInLine, msg));
    }
};

class CustomBailErrorStrategy : public antlr4::BailErrorStrategy {
public:
    void recover(antlr4::Parser* recognizer, std::exception_ptr e) override
    {
        try {
            std::rethrow_exception(e);
        } catch (const antlr4::RecognitionException& ex) {
            std::string msg = std::format("Parse error at line {}, char {}: {}", ex.getOffendingToken()->getLine(),
                                          ex.getOffendingToken()->getCharPositionInLine(), ex.what());
            throw antlr4::ParseCancellationException(msg);
        } catch (...) {
            antlr4::BailErrorStrategy::recover(recognizer, e);
        }
    }

    antlr4::Token* recoverInline(antlr4::Parser* recognizer) override
    {
        auto* currentToken = recognizer->getCurrentToken();
        auto expectedTokens = recognizer->getExpectedTokens();

        std::string expectedStr = "one of: ";
        auto& vocab = recognizer->getVocabulary();
        bool first = true;

        for (size_t token : expectedTokens.toList()) {
            if (!first) expectedStr += ", ";
            first = false;

            std::string tokenName = std::string(vocab.getSymbolicName(token));
            if (tokenName.empty()) {
                tokenName = std::string(vocab.getLiteralName(token));
                if (tokenName.empty()) {
                    tokenName = std::to_string(token);
                } else if (tokenName.size() >= 2 && tokenName[0] == '\'' && tokenName.back() == '\'') {
                    tokenName = tokenName.substr(1, tokenName.size() - 2);
                }
            }
            expectedStr += tokenName;
        }

        std::string foundToken = currentToken->getText();
        std::string msg
            = std::format("Parse error at line {}, char {}: expected {}, but found '{}'", currentToken->getLine(),
                          currentToken->getCharPositionInLine(), expectedStr, foundToken);

        throw antlr4::ParseCancellationException(msg);
    }
};

ModuleCompiler::ModuleCompiler(const std::string& code, const std::string& name,
                               std::optional<std::filesystem::path> entry_path)
    : code(code)
    , name(name)
    , entry_path(std::move(entry_path))
    , builder(context)
    , module(std::make_unique<llvm::Module>(name, context))
    , data_layout(module->getDataLayout())
{
    type_alias_scopes.emplace_back();
    current_module_name = "__main__";
    register_visitors();
    register_malloc();
    register_free();
}

ASTNode* ModuleCompiler::parse_program(const std::string& source)
{
    antlr4::ANTLRInputStream input(source);
    kyoto::KyotoLexer lexer(&input);
    lexer.removeErrorListeners();
    lexer.addErrorListener(new LexerErrorListener());
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();

    kyoto::KyotoParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(new ParserErrorListener());
    parser.setBuildParseTree(true);
    parser.setErrorHandler(std::make_unique<CustomBailErrorStrategy>());
    auto* tree = parser.program();

    ASTBuilderVisitor visitor(*this);
    return std::any_cast<ASTNode*>(visitor.visit(tree));
}

void ModuleCompiler::enter_module_context(const std::string& module_name)
{
    current_module_name = module_name;
    const auto it = loaded_modules.find(module_name);
    if (it != loaded_modules.end()) {
        current_source_path = it->second.path;
        code = it->second.code;
    } else {
        current_source_path.clear();
    }
}

std::string ModuleCompiler::mangle_module_name(const std::string& module_name) const
{
    std::string mangled = module_name;
    std::replace(mangled.begin(), mangled.end(), '.', '_');
    return mangled;
}

std::string ModuleCompiler::make_qualified_name(const std::string& module_name, const std::string& entity_name) const
{
    if (module_name.empty()) return entity_name;
    return mangle_module_name(module_name) + "__" + entity_name;
}

std::string ModuleCompiler::qualify_local_name(const std::string& entity_name) const
{
    return make_qualified_name(current_module_name, entity_name);
}

std::string ModuleCompiler::resolve_module_reference(const std::string& module_name) const
{
    if (!is_imported_module_visible(module_name)) {
        throw std::runtime_error(
            std::format("Module `{}` is not imported into `{}`", module_name, current_module_name));
    }
    return module_name;
}

std::string ModuleCompiler::qualify_imported_name(const std::string& module_name, const std::string& entity_name) const
{
    return make_qualified_name(resolve_module_reference(module_name), entity_name);
}

bool ModuleCompiler::is_imported_module_visible(const std::string& module_name) const
{
    const auto it = module_imports.find(current_module_name);
    if (it == module_imports.end()) return false;
    return it->second.contains(module_name);
}

std::vector<std::string> ModuleCompiler::parse_imports(const std::string& source,
                                                       const std::filesystem::path& path) const
{
    try {
        antlr4::ANTLRInputStream input(source);
        kyoto::KyotoLexer lexer(&input);
        lexer.removeErrorListeners();
        lexer.addErrorListener(new LexerErrorListener());
        antlr4::CommonTokenStream tokens(&lexer);
        tokens.fill();

        kyoto::KyotoParser parser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(new ParserErrorListener());
        parser.setBuildParseTree(true);
        parser.setErrorHandler(std::make_unique<CustomBailErrorStrategy>());
        auto* tree = parser.program();

        std::vector<std::string> imports;
        for (auto* rawTopLevel : tree->topLevel()) {
            auto* topLevel = const_cast<kyoto::KyotoParser::TopLevelContext*>(rawTopLevel);
            if (!topLevel->importStatement()) continue;

            std::string module_name;
            auto* modulePath
                = const_cast<kyoto::KyotoParser::ModulePathContext*>(topLevel->importStatement()->modulePath());
            for (size_t i = 0; i < modulePath->IDENTIFIER().size(); ++i) {
                if (!module_name.empty()) module_name += ".";
                module_name += modulePath->IDENTIFIER(i)->getText();
            }
            imports.push_back(module_name);
        }
        return imports;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("{}: {}", path.string(), e.what()));
    }
}

void ModuleCompiler::load_module_recursive(const std::string& module_name, const std::filesystem::path& path,
                                           const std::string* source, std::vector<std::string>& stack)
{
    const auto normalized_path = std::filesystem::weakly_canonical(path);

    if (std::find(stack.begin(), stack.end(), module_name) != stack.end()) {
        std::string cycle;
        for (const auto& item : stack) {
            if (!cycle.empty()) cycle += " -> ";
            cycle += item;
        }
        cycle += " -> " + module_name;
        throw std::runtime_error(std::format("Import cycle detected: {}", cycle));
    }

    const auto existing = loaded_modules.find(module_name);
    if (existing != loaded_modules.end()) {
        if (existing->second.path != normalized_path) {
            throw std::runtime_error(std::format("Module `{}` resolves to multiple files: `{}` and `{}`", module_name,
                                                 existing->second.path.string(), normalized_path.string()));
        }
        return;
    }

    std::string module_source = source ? *source : utils::File::get_source(normalized_path.string());
    stack.push_back(module_name);

    auto imports = parse_imports(module_source, normalized_path);
    loaded_modules.emplace(module_name,
                           LoadedModule { module_name, normalized_path, std::move(module_source), std::move(imports) });
    module_imports[module_name] = {};

    const auto base_dir = normalized_path.parent_path();
    const auto imported_names = loaded_modules.at(module_name).imports;
    for (const auto& imported_module : imported_names) {
        module_imports[module_name].insert(imported_module);

        std::filesystem::path imported_path = base_dir;
        std::string relative = imported_module;
        std::replace(relative.begin(), relative.end(), '.', std::filesystem::path::preferred_separator);
        imported_path /= relative + ".kyo";
        if (!std::filesystem::exists(imported_path)) {
            throw std::runtime_error(std::format("{}: imported module `{}` not found at `{}`", normalized_path.string(),
                                                 imported_module, imported_path.string()));
        }

        load_module_recursive(imported_module, imported_path, nullptr, stack);
    }

    stack.pop_back();
}

void ModuleCompiler::load_modules()
{
    loaded_modules.clear();
    module_imports.clear();

    if (!entry_path.has_value()) {
        const auto inline_path = std::filesystem::path("<memory>");
        loaded_modules.emplace("__main__",
                               LoadedModule {
                                   "__main__",
                                   inline_path,
                                   code,
                                   {},
                               });
        module_imports["__main__"] = {};
        current_source_path = inline_path;
        current_module_name = "__main__";
        return;
    }

    auto root_path = std::filesystem::weakly_canonical(*entry_path);
    std::vector<std::string> stack;
    load_module_recursive("__main__", root_path, &code, stack);
    current_source_path = root_path;
    current_module_name = "__main__";
}

std::vector<std::string> ModuleCompiler::topo_sort_modules() const
{
    enum class VisitState { Pending, Visiting, Done };

    std::unordered_map<std::string, VisitState> states;
    std::vector<std::string> ordered;
    ordered.reserve(loaded_modules.size());

    std::function<void(const std::string&)> dfs = [&](const std::string& module_name) {
        auto state = states[module_name];
        if (state == VisitState::Done) return;
        if (state == VisitState::Visiting) {
            throw std::runtime_error(std::format("Import cycle detected at `{}`", module_name));
        }

        states[module_name] = VisitState::Visiting;
        if (const auto imports_it = module_imports.find(module_name); imports_it != module_imports.end()) {
            for (const auto& dependency : imports_it->second) {
                dfs(dependency);
            }
        }

        states[module_name] = VisitState::Done;
        ordered.push_back(module_name);
    };

    dfs("__main__");
    return ordered;
}

std::unique_ptr<ASTNode> ModuleCompiler::build_module_ast(const std::string& module_name)
{
    enter_module_context(module_name);
    instantiated_nodes.clear();
    return std::unique_ptr<ASTNode>(parse_program(code));
}

void ModuleCompiler::llvm_pass()
{
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::FunctionPassManager FPM;
    FPM.addPass(FunctionTerminationPass(*this));

    llvm::ModulePassManager MPM;
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));

    MPM.run(*module, MAM);
}

void ModuleCompiler::ensure_main_fn() const
{
    const auto* main_fn = module->getFunction("main");
    if (!main_fn) {
        throw std::runtime_error("Main function not found");
    }

    if (!main_fn->getReturnType()->isIntegerTy(32)) {
        main_fn->getReturnType()->dump();
        throw std::runtime_error("Main function must return an `i32`");
    }
}

void ModuleCompiler::insert_dummy_return(llvm::BasicBlock& bb)
{
    const auto* llvm_type = bb.getParent()->getReturnType();

    if (!llvm_type) return;

    builder.SetInsertPoint(&bb);

    if (llvm_type->isVoidTy()) {
        builder.CreateRetVoid();
    } else if (llvm_type->isIntegerTy()) {
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(llvm_type->getIntegerBitWidth(), 0)));
    } else {
        std::cerr << "Error: unsupported return type\n";
        std::exit(1);
    }
}

llvm::BasicBlock* ModuleCompiler::create_basic_block(const std::string& name)
{
    return llvm::BasicBlock::Create(context, name, builder.GetInsertBlock()->getParent());
}

std::optional<std::string> ModuleCompiler::gen_ir()
{
    auto report_error = [](const std::string& msg) {
        constexpr auto* RED = "\033[0;31m";
        constexpr auto* NC = "\033[0m";
        std::cerr << RED << "Error: " << NC << msg << std::endl;
    };

    try {
        load_modules();

        std::vector<std::unique_ptr<ASTNode>> module_asts;
        const auto module_order = topo_sort_modules();
        module_asts.reserve(module_order.size());

        for (const auto& module_name : module_order) {
            auto ast = build_module_ast(module_name);
            module_asts.push_back(std::move(ast));
        }

        for (size_t i = 0; i < module_order.size(); ++i) {
            enter_module_context(module_order[i]);
            for (auto& visitor : analysis_visitors) {
                visitor->visit(module_asts[i].get());
            }
        }

        for (size_t i = 0; i < module_order.size(); ++i) {
            enter_module_context(module_order[i]);
            module_asts[i]->gen();
        }

        llvm_pass();
        ensure_main_fn();
    } catch (const antlr4::ParseCancellationException& e) {
        report_error(e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        report_error(e.what());
        return std::nullopt;
    } catch (...) {
        report_error("Unknown exception");
        return std::nullopt;
    }

    std::string llvm_err;
    llvm::raw_string_ostream err(llvm_err);
    if (!verify_module(err)) {
        std::cerr << "Error: " << err.str() << std::endl;
        return std::nullopt;
    }

    std::string llvm_ir;
    llvm::raw_string_ostream os(llvm_ir);
    module->print(os, nullptr);
    return os.str();
}

std::optional<Symbol> ModuleCompiler::get_symbol(const std::string& name)
{
    return symbol_table.get_symbol(name);
}

void ModuleCompiler::add_symbol(const std::string& name, Symbol symbol)
{
    symbol_table.add_symbol(name, symbol);
}

void ModuleCompiler::add_function(FunctionNode* node)
{
    std::string key = make_function_key(node->get_name(), node->get_params().size());
    functions[key] = node;
}

std::optional<FunctionNode*> ModuleCompiler::get_function(const std::string& name)
{
    for (const auto& [key, func] : functions) {
        if (key.find(name + "_") == 0) return func;
    }
    return std::nullopt;
}

std::optional<FunctionNode*> ModuleCompiler::get_function(const std::string& name, size_t arity)
{
    std::string key = make_function_key(name, arity);
    if (functions.contains(key)) return functions[key];
    return std::nullopt;
}

void ModuleCompiler::register_malloc()
{
    auto* ret_type = new PointerType(new PrimitiveType(PrimitiveType::Kind::I8));
    FunctionNode::Parameter param;
    param.name = "size";
    param.type = new PrimitiveType(PrimitiveType::Kind::I64);
    std::vector<FunctionNode::Parameter> args = { param };
    auto* malloc = new FunctionNode("malloc", args, false, ret_type, nullptr, *this, true, "malloc");
    add_function(malloc);
    (void)malloc->gen_prototype();
}

void ModuleCompiler::register_free()
{
    auto* ret_type = KType::get_void();
    FunctionNode::Parameter param;
    param.name = "ptr";
    param.type = new PointerType(new PrimitiveType(PrimitiveType::Kind::I8));
    std::vector<FunctionNode::Parameter> args = { param };
    auto* free = new FunctionNode("free", args, false, ret_type, nullptr, *this, true, "free");
    add_function(free);
    (void)free->gen_prototype();
}

void ModuleCompiler::register_visitors()
{
    analysis_visitors.push_back(std::make_unique<ConstructorIdentifierVisitor>(*this));
    analysis_visitors.push_back(std::make_unique<ClassIdentifierVisitor>(*this));
    analysis_visitors.push_back(std::make_unique<FunctionIdentifierVisitor>(*this));
}

void ModuleCompiler::push_scope()
{
    symbol_table.push_scope();

    type_alias_scopes.emplace_back();

    // We insert function arguments into the symbol table. This is once we are
    // in the second scope as the first scope is the global scope.
    if (symbol_table.n_scopes() == 2) {
        int i = 0;
        for (auto iter = current_fn->arg_begin(); iter != current_fn->arg_end(); iter++) {
            llvm::Value* arg = &*iter;
            auto arg_name = current_fn_node->get_params()[i++].name;
            auto* arg_type = arg->getType();
            auto* arg_alloc = builder.CreateAlloca(arg_type, nullptr, arg_name);
            builder.CreateStore(arg, arg_alloc);
            symbol_table.add_symbol(arg_name, Symbol { arg_alloc, current_fn_node->get_params()[i - 1].type->copy() });
        }
    }
}

void ModuleCompiler::pop_scope()
{
    // destruct the types of the function arguments. `2` is the global scope and
    // the function scope. Whene we pop the function, we destruct the types of
    // the function arguments.
    if (symbol_table.n_scopes() == 2) {
        int i = 0;
        for (auto iter = current_fn->arg_begin(); iter != current_fn->arg_end(); iter++) {
            auto arg_name = current_fn_node->get_params()[i++].name;
            auto s = symbol_table.get_symbol(arg_name);
            assert(s.has_value() && "Expected function parameter symbol to be in the symbol table");
            delete s.value().type;
        }
    }

    symbol_table.pop_scope();

    if (!type_alias_scopes.empty()) type_alias_scopes.pop_back();
}

FunctionNode* ModuleCompiler::get_current_function_node() const
{
    return current_fn_node;
}

void ModuleCompiler::set_current_function(FunctionNode* node, llvm::Function* func)
{
    current_fn_node = node;
    current_fn = func;
}

void ModuleCompiler::push_class(std::string name)
{
    current_class = std::move(name);
    classes.insert(current_class);
}

void ModuleCompiler::pop_class()
{
    current_class = "";
}

std::string ModuleCompiler::get_current_class() const
{
    return current_class;
}

bool ModuleCompiler::class_exists(const std::string& name) const
{
    return classes.contains(name);
}

size_t ModuleCompiler::get_primitive_size(const std::string& name) const
{
    if (name == "I8") return 1;
    if (name == "Char") return 1;
    if (name == "Boolean") return 1;
    if (name == "I16") return 2;
    if (name == "I32") return 4;
    if (name == "I64") return 8;
    if (name == "F32") return 4;
    if (name == "F64") return 8;

    throw std::runtime_error(std::format("Unknown size for type: {}", name));
}

size_t ModuleCompiler::get_type_size(const std::string& name) const
{
    if (classes_metadata.contains(name)) {
        llvm::StructType* struct_type = classes_metadata.at(name).llvm_type;
        return data_layout.getStructLayout(struct_type)->getSizeInBytes();
    }

    return get_primitive_size(name);
}

void ModuleCompiler::add_class_metadata(const std::string& name, const ClassMetadata& data)
{
    classes_metadata[name] = data;
}

llvm::StructType* ModuleCompiler::get_llvm_struct(const std::string& name) const
{
    if (!classes_metadata.contains(name)) {
        throw std::runtime_error(std::format("Class metadata for '{}' not found", name));
    }
    return classes_metadata.at(name).llvm_type;
}

ClassMetadata& ModuleCompiler::get_class_metadata(const std::string& name)
{
    if (!classes_metadata.contains(name)) {
        throw std::runtime_error(std::format("Class '{}' not found", name));
    }
    return classes_metadata.at(name);
}

void ModuleCompiler::push_fn_return_type(KType* type)
{
    curr_fn_ret_type = type;
}

void ModuleCompiler::pop_fn_return_type()
{
    curr_fn_ret_type = nullptr;
}

KType* ModuleCompiler::get_fn_return_type() const
{
    return curr_fn_ret_type;
}

std::string ModuleCompiler::make_function_key(const std::string& name, size_t arity) const
{
    return name + "_" + std::to_string(arity);
}

size_t ModuleCompiler::n_scopes() const
{
    return symbol_table.n_scopes();
}

bool ModuleCompiler::verify_module(llvm::raw_string_ostream& os) const
{
    auto err = llvm::verifyModule(*module, &os);
    os.flush();
    return err == false;
}

void ModuleCompiler::register_type_alias(const std::string& alias, KType* type)
{
    if (building_top_level) {
        module_type_aliases[current_module_name][alias] = std::unique_ptr<KType>(type->copy());
        return;
    }

    if (!type_alias_scopes.empty()) {
        type_alias_scopes.back()[alias] = type;
    }
}

KType* ModuleCompiler::resolve_type_alias(const std::string& alias)
{
    for (auto it = type_alias_scopes.rbegin(); it != type_alias_scopes.rend(); ++it) {
        auto found = it->find(alias);
        if (found != it->end()) return found->second;
    }

    if (const auto module_it = module_type_aliases.find(current_module_name); module_it != module_type_aliases.end()) {
        if (const auto alias_it = module_it->second.find(alias); alias_it != module_it->second.end()) {
            return alias_it->second.get();
        }
    }

    return nullptr;
}

KType* ModuleCompiler::resolve_type_alias(const std::string& module_name, const std::string& alias)
{
    const auto resolved_module = resolve_module_reference(module_name);
    if (const auto module_it = module_type_aliases.find(resolved_module); module_it != module_type_aliases.end()) {
        if (const auto alias_it = module_it->second.find(alias); alias_it != module_it->second.end()) {
            return alias_it->second.get();
        }
    }
    return nullptr;
}

bool ModuleCompiler::is_type_alias(const std::string& name) const
{
    for (auto it = type_alias_scopes.rbegin(); it != type_alias_scopes.rend(); ++it) {
        if (it->find(name) != it->end()) return true;
    }

    if (const auto module_it = module_type_aliases.find(current_module_name); module_it != module_type_aliases.end()) {
        return module_it->second.contains(name);
    }

    return false;
}

void ModuleCompiler::push_type_alias_scope()
{
    type_alias_scopes.emplace_back();
}

void ModuleCompiler::pop_type_alias_scope()
{
    if (!type_alias_scopes.empty()) type_alias_scopes.pop_back();
}

void ModuleCompiler::instantiate_template(const std::string& name, const std::string& mangled_name,
                                          const std::string& type_str)
{
    if (classes.contains(mangled_name)) return;
    if (std::find_if(instantiated_nodes.begin(), instantiated_nodes.end(),
                     [&](ASTNode* n) {
                         auto* cdn = dynamic_cast<ClassDefinitionNode*>(n);
                         return cdn && cdn->get_name() == mangled_name;
                     })
        != instantiated_nodes.end()) {
        return;
    }

    if (!template_registry.contains(name)) throw std::runtime_error("Template " + name + " not found");

    classes.insert(mangled_name);

    auto& tmpl = template_registry[name];
    std::string text = tmpl.raw_text;
    std::string source_template_name = name;
    if (const auto pos = source_template_name.rfind("__"); pos != std::string::npos) {
        source_template_name = source_template_name.substr(pos + 2);
    }

    std::regex reg("\\b" + tmpl.param + "\\b");

    std::string class_decl = "class\\s+" + source_template_name + "\\s*<\\s*" + tmpl.param + "\\s*>";
    std::regex class_reg(class_decl);
    std::string new_text = std::regex_replace(text, class_reg, "class " + mangled_name);
    new_text = std::regex_replace(new_text, reg, type_str);

    antlr4::ANTLRInputStream input(new_text);
    kyoto::KyotoLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    kyoto::KyotoParser parser(&tokens);
    parser.removeErrorListeners();
    auto tree = parser.classDefinition();

    ASTBuilderVisitor visitor(*this);
    auto node = std::any_cast<ASTNode*>(visitor.visitClassDefinition(tree));

    instantiated_nodes.push_back(node);
}
