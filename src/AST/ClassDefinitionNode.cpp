#include "kyoto/AST/ClassDefinitionNode.h"

#include <iostream>
#include <sstream>
#include <utility>

ClassDefinitionNode::ClassDefinitionNode(std::string name, std::vector<ASTNode*> components, ModuleCompiler& compiler)
    : name(std::move(name))
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
    ss << "class " << name << " {\n";
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
