#pragma once

#include <any>

#include "KyotoParserBaseVisitor.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class ASTBuilderVisitor : public kyoto::KyotoParserBaseVisitor {
public:
    ASTBuilderVisitor(ModuleCompiler& compiler);
    std::any visitProgram(kyoto::KyotoParser::ProgramContext* ctx) override;
    std::any visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx) override;
    std::any visitBlock(kyoto::KyotoParser::BlockContext* ctx) override;
    std::any visitExpressionStatement(kyoto::KyotoParser::ExpressionStatementContext* ctx) override;
    std::any visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx) override;
    std::any visitRegularDeclaration(kyoto::KyotoParser::RegularDeclarationContext* ctx) override;
    std::any visitTypelessDeclaration(kyoto::KyotoParser::TypelessDeclarationContext* ctx) override;
    std::any visitAssignmentExpression(kyoto::KyotoParser::AssignmentExpressionContext* ctx) override;
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

    std::any visitLessThanExpression(kyoto::KyotoParser::LessThanExpressionContext* ctx) override;
    std::any visitGreaterThanExpression(kyoto::KyotoParser::GreaterThanExpressionContext* ctx) override;
    std::any visitLessThanOrEqualExpression(kyoto::KyotoParser::LessThanOrEqualExpressionContext* ctx) override;
    std::any visitGreaterThanOrEqualExpression(kyoto::KyotoParser::GreaterThanOrEqualExpressionContext* ctx) override;
    std::any visitEqualsExpression(kyoto::KyotoParser::EqualsExpressionContext* ctx) override;
    std::any visitNotEqualsExpression(kyoto::KyotoParser::NotEqualsExpressionContext* ctx) override;

    std::any visitLogicalAndExpression(kyoto::KyotoParser::LogicalAndExpressionContext* ctx) override;
    std::any visitLogicalOrExpression(kyoto::KyotoParser::LogicalOrExpressionContext* ctx) override;

    std::any visitIfStatement(kyoto::KyotoParser::IfStatementContext* ctx) override;

    std::any visitForStatement(kyoto::KyotoParser::ForStatementContext* ctx) override;
    std::any visitForInit(kyoto::KyotoParser::ForInitContext* ctx) override;
    std::any visitForCondition(kyoto::KyotoParser::ForConditionContext* ctx) override;
    std::any visitForUpdate(kyoto::KyotoParser::ForUpdateContext* ctx) override;

private:
    PrimitiveType::Kind parse_primitive_type(const std::string& type) const;
    std::optional<int64_t> parse_signed_integer_into(const std::string& str, const PrimitiveType::Kind kind) const;
    std::optional<double> parse_double(const std::string& str) const;
    std::optional<float> parse_float(const std::string& str) const;
    std::optional<bool> parse_bool(const std::string& str) const;

private:
    ModuleCompiler& compiler;
};