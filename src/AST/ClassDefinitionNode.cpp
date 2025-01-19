#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/KType.h"

#include <iostream>
#include <sstream>
#include <utility>

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
    std::cout << to_string() << std::endl;
    return nullptr;
}

ConstructorNode::ConstructorNode(std::string name, std::vector<FunctionNode::Parameter> args, ASTNode* body,
                                 ModuleCompiler& compiler)
    : FunctionNode(std::move(name), std::move(args), false, KType::get_void(), body, compiler)
{
    FunctionNode::insert_arg({ "self", new PointerType(new ClassType(name)) }, 0);
}

ConstructorNode::~ConstructorNode() = default;
