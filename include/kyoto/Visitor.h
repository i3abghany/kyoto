#pragma once

#include <any>

#include "KyotoParserBaseVisitor.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class ASTBuilderVisitor final : public kyoto::KyotoParserBaseVisitor {
public:
    explicit ASTBuilderVisitor(ModuleCompiler& compiler);
    std::any visitProgram(kyoto::KyotoParser::ProgramContext* ctx) override;
    std::any visitCdecl(kyoto::KyotoParser::CdeclContext* ctx) override;
    std::any visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx) override;
    std::any visitBlock(kyoto::KyotoParser::BlockContext* ctx) override;
    std::any visitExpressionStatement(kyoto::KyotoParser::ExpressionStatementContext* ctx) override;
    std::any visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx) override;
    std::any visitRegularDeclaration(kyoto::KyotoParser::RegularDeclarationContext* ctx) override;
    std::any visitTypelessDeclaration(kyoto::KyotoParser::TypelessDeclarationContext* ctx) override;
    std::any visitAssignmentExpression(kyoto::KyotoParser::AssignmentExpressionContext* ctx) override;
    std::any visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx) override;

    std::any visitFunctionCallExpression(kyoto::KyotoParser::FunctionCallExpressionContext* ctx) override;
    std::any visitStringExpression(kyoto::KyotoParser::StringExpressionContext* ctx) override;
    std::any visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx) override;
    std::any visitCharExpression(kyoto::KyotoParser::CharExpressionContext* ctx) override;
    std::any visitIdentifierExpression(kyoto::KyotoParser::IdentifierExpressionContext* ctx) override;
    std::any visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx) override;
    std::any visitArrayExpression(kyoto::KyotoParser::ArrayExpressionContext* ctx) override;

    std::any visitAddressOfExpression(kyoto::KyotoParser::AddressOfExpressionContext* ctx) override;
    std::any visitDereferenceExpression(kyoto::KyotoParser::DereferenceExpressionContext* ctx) override;

    std::any visitMemberAccessExpression(kyoto::KyotoParser::MemberAccessExpressionContext* ctx) override;
    std::any visitMethodCallExpression(kyoto::KyotoParser::MethodCallExpressionContext* ctx) override;

    std::any visitPrefixIncrementExpression(kyoto::KyotoParser::PrefixIncrementExpressionContext* ctx) override;
    std::any visitPrefixDecrementExpression(kyoto::KyotoParser::PrefixDecrementExpressionContext* ctx) override;

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

    std::any visitClassDefinition(kyoto::KyotoParser::ClassDefinitionContext* ctx) override;
    std::any visitClassComponent(kyoto::KyotoParser::ClassComponentContext* ctx) override;
    std::any visitConstructorDefinition(kyoto::KyotoParser::ConstructorDefinitionContext* ctx) override;

    std::any visitBoolType(kyoto::KyotoParser::BoolTypeContext* ctx) override;
    std::any visitCharType(kyoto::KyotoParser::CharTypeContext* ctx) override;
    std::any visitI8Type(kyoto::KyotoParser::I8TypeContext* ctx) override;
    std::any visitI16Type(kyoto::KyotoParser::I16TypeContext* ctx) override;
    std::any visitI32Type(kyoto::KyotoParser::I32TypeContext* ctx) override;
    std::any visitI64Type(kyoto::KyotoParser::I64TypeContext* ctx) override;
    std::any visitF32Type(kyoto::KyotoParser::F32TypeContext* ctx) override;
    std::any visitF64Type(kyoto::KyotoParser::F64TypeContext* ctx) override;
    std::any visitStrType(kyoto::KyotoParser::StrTypeContext* ctx) override;
    std::any visitVoidType(kyoto::KyotoParser::VoidTypeContext* ctx) override;
    std::any visitPointerType(kyoto::KyotoParser::PointerTypeContext* ctx) override;
    std::any visitArrayType(kyoto::KyotoParser::ArrayTypeContext* ctx) override;
    std::any visitClassType(kyoto::KyotoParser::ClassTypeContext* ctx) override;

private:
    [[nodiscard]] std::optional<int64_t> parse_signed_integer_into(const std::string& str,
                                                                   PrimitiveType::Kind kind) const;
    [[nodiscard]] std::optional<int64_t> parse_bool(const std::string& str) const;

private:
    ModuleCompiler& compiler;
};
