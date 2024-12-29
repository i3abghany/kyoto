#pragma once

#include <any>

#include "KyotoParserBaseVisitor.h"
#include "kyoto/AST/ASTNode.h"

class ModuleCompiler;

class ASTBuilderVisitor : public kyoto::KyotoParserBaseVisitor {
public:
    ASTBuilderVisitor(ModuleCompiler& compiler);
    std::any visitProgram(kyoto::KyotoParser::ProgramContext* ctx) override;
    std::any visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx) override;
    std::any visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx) override;
    std::any visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx) override;
    std::any visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx) override;
    std::any visitUnaryExpression(kyoto::KyotoParser::UnaryExpressionContext* ctx) override;
    std::any visitAdditiveExpression(kyoto::KyotoParser::AdditiveExpressionContext* ctx) override;
    std::any visitMultiplicativeExpression(kyoto::KyotoParser::MultiplicativeExpressionContext* ctx) override;
    std::any visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx) override;

private:
    ModuleCompiler& compiler;
};