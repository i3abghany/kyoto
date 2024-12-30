#include <any>
#include <string>
#include <vector>

#include "KyotoParser.h"
#include "tree/TerminalNode.h"

#include "kyoto/AST/ASTBinaryArithNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/ModuleCompiler.h"
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
    compiler.push_scope();
    for (auto stmt : ctx->block()->statement()) {
        body.push_back(std::any_cast<ASTNode*>(visit(stmt)));
    }
    compiler.pop_scope();
    return (ASTNode*)new FunctionNode(name, args, ret_type, body, compiler);
}

std::any ASTBuilderVisitor::visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx)
{
    std::string type = ctx->type()->getText();
    std::string name = ctx->IDENTIFIER()->getText();
    return (ASTNode*)new DeclarationStatementNode(name, type, compiler);
}

std::any ASTBuilderVisitor::visitFullDeclaration(kyoto::KyotoParser::FullDeclarationContext* ctx)
{
    std::string type = ctx->type()->getText();
    std::string name = ctx->IDENTIFIER()->getText();
    auto* expr = std::any_cast<ASTNode*>(visit(ctx->expression()));
    return (ASTNode*)new FullDeclarationStatementNode(name, type, expr, compiler);
}

std::any ASTBuilderVisitor::visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx)
{
    auto* expr = std::any_cast<ASTNode*>(visit(ctx->expression()));
    return (ASTNode*)new ReturnStatementNode(expr, compiler);
}

std::any ASTBuilderVisitor::visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx)
{
    // FIXME: only accounts for 4-byte signed integers
    return (ASTNode*)new IntNode(std::stoi(ctx->getText()), 4, true, compiler);
}

std::any ASTBuilderVisitor::visitIdentifierExpression(kyoto::KyotoParser::IdentifierExpressionContext* ctx)
{
    return (ASTNode*)new IdentifierExpressionNode(ctx->IDENTIFIER()->getText(), compiler);
}

std::any ASTBuilderVisitor::visitPositiveExpression(kyoto::KyotoParser::PositiveExpressionContext* ctx)
{
    return visit(ctx->expression());
}

std::any ASTBuilderVisitor::visitNegationExpression(kyoto::KyotoParser::NegationExpressionContext* ctx)
{
    auto* expr = std::any_cast<ASTNode*>(visit(ctx->expression()));
    return (ASTNode*)new UnaryNode(expr, "-", compiler);
}

std::any ASTBuilderVisitor::visitMultiplicationExpression(kyoto::KyotoParser::MultiplicationExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->children[2]));
    return (ASTNode*)new MulNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitDivisionExpression(kyoto::KyotoParser::DivisionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->children[2]));
    return (ASTNode*)new DivNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitModulusExpression(kyoto::KyotoParser::ModulusExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->children[2]));
    return (ASTNode*)new ModNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitAdditionExpression(kyoto::KyotoParser::AdditionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->children[2]));
    return (ASTNode*)new AddNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitSubtractionExpression(kyoto::KyotoParser::SubtractionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ASTNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ASTNode*>(visit(ctx->children[2]));
    return (ASTNode*)new SubNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx)
{
    return visit(ctx->expression());
}

PrimitiveType::Kind ASTBuilderVisitor::parse_primitive_type(const std::string& type)
{
    if (type == "bool")
        return PrimitiveType::Kind::Boolean;
    if (type == "char")
        return PrimitiveType::Kind::Char;
    if (type == "i8")
        return PrimitiveType::Kind::I8;
    if (type == "i16")
        return PrimitiveType::Kind::I16;
    if (type == "i32")
        return PrimitiveType::Kind::I32;
    if (type == "i64")
        return PrimitiveType::Kind::I64;
    if (type == "u8")
        return PrimitiveType::Kind::U8;
    if (type == "u16")
        return PrimitiveType::Kind::U16;
    if (type == "u32")
        return PrimitiveType::Kind::U32;
    if (type == "u64")
        return PrimitiveType::Kind::U64;
    if (type == "f32")
        return PrimitiveType::Kind::F32;
    if (type == "f64")
        return PrimitiveType::Kind::F64;
    if (type == "string")
        return PrimitiveType::Kind::String;
    if (type == "void")
        return PrimitiveType::Kind::Void;
    return PrimitiveType::Kind::Unknown;
}