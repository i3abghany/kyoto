#pragma once

class ASTNode;
class FunctionCall;
class ModuleCompiler;

// ConstructorResolver is a visitor that visits each node, and if it's a
// function call node, it checks if it's a constructor call, and sets the flag
// accordingly.

class ConstructorResolver {
public:
    explicit ConstructorResolver(ModuleCompiler& compiler);

    void visit(ASTNode* node);
    void visit(FunctionCall* node);

private:
    ModuleCompiler& compiler;
};