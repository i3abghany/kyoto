#include "kyoto/AST/ClassDefinitionNode.h"

#include <sstream>
#include <utility>

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
            auto* fn = dynamic_cast<FunctionNode*>(component);
            fn->insert_arg({ "self", new PointerType(new ClassType(name)) }, 0);
            component->gen();
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
