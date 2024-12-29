#include <any>

#include "kyoto/Visitor.h"

ASTBuilderVisitor::ASTBuilderVisitor(ModuleCompiler& compiler)
    : compiler(compiler)
{
}

std::any ASTBuilderVisitor::visitProgram(kyoto::KyotoParser::ProgramContext* ctx)
{
    std::vector<ASTNode*> nodes;
    for (auto node : ctx->topLevel()) {
        std::any result = visit(node);
        nodes.push_back(std::any_cast<ASTNode*>(result));
    }
    return (ASTNode*)new ProgramNode(nodes, compiler);
}

std::any ASTBuilderVisitor::visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx)
{
    std::string name = ctx->IDENTIFIER()->getText();
    std::vector<FunctionNode::Parameter> args;

    for (auto paramCtx : ctx->parameterList()->parameter()) {
        args.push_back({ paramCtx->IDENTIFIER()->getText(), paramCtx->type()->getText() });
    }

    std::string ret_type = ctx->type() ? ctx->type()->getText() : "void";
    std::vector<ASTNode*> body;
    for (auto stmt : ctx->block()->statement()) {
        body.push_back(std::any_cast<ASTNode*>(visit(stmt)));
    }
    return (ASTNode*)new FunctionNode(name, args, ret_type, body, compiler);
}

std::any ASTBuilderVisitor::visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx)
{
    std::string type = ctx->type()->getText();
    std::string name = ctx->IDENTIFIER()->getText();
    return (ASTNode*)new DeclarationStatementNode(name, type, compiler);
}

std::any ASTBuilderVisitor::visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx)
{
    auto* expr = std::any_cast<ASTNode*>(visit(ctx->expression()));
    return (ASTNode*)new ReturnStatementNode(expr, compiler);
}

std::any ASTBuilderVisitor::visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx)
{
    return (ASTNode*)new IntNode(std::stoi(ctx->getText()), 4, compiler);
}

std::any ASTBuilderVisitor::visitUnaryExpression(kyoto::KyotoParser::UnaryExpressionContext* ctx)
{
    auto* expr = std::any_cast<ASTNode*>(visit(ctx->expression()));
    return (ASTNode*)new UnaryNode(expr, ctx->unaryOp()->getText(), compiler);
}

std::any ASTBuilderVisitor::visitAdditiveExpression(kyoto::KyotoParser::AdditiveExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->expression(0)));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->expression(1)));
    return (ASTNode*)new AddNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitMultiplicativeExpression(kyoto::KyotoParser::MultiplicativeExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->expression(0)));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->expression(1)));
    return (ASTNode*)new MulNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx)
{
    return visit(ctx->expression());
}