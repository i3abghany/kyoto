#include "kyoto/AST/ClassDefinitionNode.h"

#include <sstream>
#include <utility>

#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/KType.h"

ClassDefinitionNode::ClassDefinitionNode(std::string name, std::string parent, std::vector<ASTNode*> components,
                                         ModuleCompiler& compiler)
    : name(std::move(name))
    , parent(std::move(parent))
    , components(std::move(components))
    , compiler(compiler)
{
}

ClassDefinitionNode::~ClassDefinitionNode()
{
    for (const auto& component : components) {
        delete component;
    }
}

std::string ClassDefinitionNode::to_string() const
{
    std::ostringstream ss;
    ss << "class " << name;
    if (!parent.empty()) ss << " : " << parent;
    ss << " {\n";
    for (const auto& component : components) {
        ss << '\t' << component->to_string() << std::endl;
    }
    ss << "};\n";
    return ss.str();
}

llvm::Value* ClassDefinitionNode::gen()
{
    for (const auto& component : components) {
        if (component->is<FunctionNode>()) {
            component->gen();
        }
    }
    return nullptr;
}

const ASTNode* ClassDefinitionNode::get_declaration_of(const std::string& name) const
{
    for (const auto& component : components) {
        if (component->is<FullDeclarationStatementNode>() || component->is<DeclarationStatementNode>()) {
            return component;
        }
    }
    return nullptr;
}

ConstructorNode::ConstructorNode(std::string name, std::vector<FunctionNode::Parameter> args, ASTNode* body,
                                 ModuleCompiler& compiler)
    : FunctionNode(std::move(name), std::move(args), false, KType::get_void(), body, compiler)
{
}

ConstructorNode::~ConstructorNode() = default;

llvm::Value* ConstructorNode::gen()
{
    static constexpr auto constructor_suffix = "_constructor";
    assert(get_name().substr(get_name().size() - strlen(constructor_suffix)) == constructor_suffix);
    auto stripped_name = get_name().substr(0, get_name().size() - strlen(constructor_suffix));

    if (get_params().empty()) {
        throw std::runtime_error(fmt::format("Constructor for `{}` expects at least one `self: {}*` parameter",
                                             stripped_name, stripped_name));
    }

    auto& self = get_params().front();
    if (self.name != "self" || !self.type->is_pointer_to_class(stripped_name)) {
        throw std::runtime_error(
            fmt::format("First argument to constructor `{}` must be `self: {}*`", stripped_name, stripped_name));
    }

    return FunctionNode::gen();
}