#pragma once

#include <any>

#include "KyotoParserBaseVisitor.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/Type.h"

class ModuleCompiler;

class ASTBuilderVisitor : public kyoto::KyotoParserBaseVisitor {
public:
    ASTBuilderVisitor(ModuleCompiler& compiler);
    std::any visitProgram(kyoto::KyotoParser::ProgramContext* ctx) override;
    std::any visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx) override;
    std::any visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx) override;
    std::any visitFullDeclaration(kyoto::KyotoParser::FullDeclarationContext* ctx) override;
    std::any visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx) override;
    std::any visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx) override;
    std::any visitIdentifierExpression(kyoto::KyotoParser::IdentifierExpressionContext* ctx) override;
    std::any visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx) override;
    std::any visitNegationExpression(kyoto::KyotoParser::NegationExpressionContext* ctx) override;
    std::any visitPositiveExpression(kyoto::KyotoParser::PositiveExpressionContext* ctx) override;
    std::any visitMultiplicationExpression(kyoto::KyotoParser::MultiplicationExpressionContext* ctx) override;
    std::any visitDivisionExpression(kyoto::KyotoParser::DivisionExpressionContext* ctx) override;
    std::any visitModulusExpression(kyoto::KyotoParser::ModulusExpressionContext* ctx) override;
    std::any visitAdditionExpression(kyoto::KyotoParser::AdditionExpressionContext* ctx) override;
    std::any visitSubtractionExpression(kyoto::KyotoParser::SubtractionExpressionContext* ctx) override;

private:
    PrimitiveType::Kind parse_primitive_type(const std::string& type);

private:
    ModuleCompiler& compiler;
};