#include <any>
#include <array>
#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include "KyotoParser.h"
#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ControlFlowNodes.h"
#include "kyoto/AST/NumberNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "kyoto/Visitor.h"
#include "tree/TerminalNode.h"

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
        auto type_str = paramCtx->type()->getText();
        // FIXME: only accounts for primitive types
        auto type = parse_primitive_type(type_str);
        args.push_back({ paramCtx->IDENTIFIER()->getText(), new PrimitiveType(type) });
    }

    std::string ret_type_str = ctx->type() ? ctx->type()->getText() : "void";
    auto ret_type = new PrimitiveType(parse_primitive_type(ret_type_str));
    auto body = std::any_cast<ASTNode*>(visit(ctx->block()));
    return (ASTNode*)new FunctionNode(name, args, ret_type, body, compiler);
}

std::any ASTBuilderVisitor::visitBlock(kyoto::KyotoParser::BlockContext* ctx)
{
    std::vector<ASTNode*> nodes;
    for (auto stmt : ctx->statement()) {
        nodes.push_back(std::any_cast<ASTNode*>(visit(stmt)));
    }
    return (ASTNode*)new BlockNode(nodes, compiler);
}

std::any ASTBuilderVisitor::visitExpressionStatement(kyoto::KyotoParser::ExpressionStatementContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ASTNode*)new ExpressionStatementNode(expr, compiler);
}

std::any ASTBuilderVisitor::visitDeclaration(kyoto::KyotoParser::DeclarationContext* ctx)
{
    std::string type_str = ctx->type()->getText();
    auto type = new PrimitiveType(parse_primitive_type(type_str));
    std::string name = ctx->IDENTIFIER()->getText();
    return (ASTNode*)new DeclarationStatementNode(name, type, compiler);
}

std::any ASTBuilderVisitor::visitFullDeclaration(kyoto::KyotoParser::FullDeclarationContext* ctx)
{
    std::string type_str = ctx->type()->getText();
    auto type = new PrimitiveType(parse_primitive_type(type_str));
    std::string name = ctx->IDENTIFIER()->getText();
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ASTNode*)new FullDeclarationStatementNode(name, type, expr, compiler);
}

std::any ASTBuilderVisitor::visitAssignmentExpression(kyoto::KyotoParser::AssignmentExpressionContext* ctx)
{
    auto* lhs_expr = dynamic_cast<kyoto::KyotoParser::IdentifierExpressionContext*>(ctx->expression(0));
    if (!lhs_expr) {
        throw std::runtime_error(fmt::format("Expected an identifier on the left-hand side of the assignment. Got `{}`",
                                             ctx->expression(0)->getText()));
    }

    auto name = lhs_expr->IDENTIFIER()->getText();
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression(1)));
    return (ExpressionNode*)new AssignmentNode(name, expr, compiler);
}

std::any ASTBuilderVisitor::visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ASTNode*)new ReturnStatementNode(expr, compiler);
}

std::any ASTBuilderVisitor::visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx)
{
    auto txt = ctx->getText();

    // The resulting node will have the smallest possible integer type.
    // Terminate if we can't parse the number.

    using Kind = PrimitiveType::Kind;
    std::array kinds = { Kind::I8, Kind::I16, Kind::I32, Kind::I64, Kind::Boolean };

    for (auto kind : kinds) {
        if (auto num = parse_signed_integer_into(txt, kind); num.has_value())
            return (ExpressionNode*)new NumberNode(num.value(), new PrimitiveType(kind), compiler);
    }

    assert(false && "Unknown Integer type");
}

std::any ASTBuilderVisitor::visitIdentifierExpression(kyoto::KyotoParser::IdentifierExpressionContext* ctx)
{
    return (ExpressionNode*)new IdentifierExpressionNode(ctx->IDENTIFIER()->getText(), compiler);
}

std::any ASTBuilderVisitor::visitPositiveExpression(kyoto::KyotoParser::PositiveExpressionContext* ctx)
{
    return visit(ctx->expression());
}

std::any ASTBuilderVisitor::visitNegationExpression(kyoto::KyotoParser::NegationExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, "-", compiler);
}

std::any ASTBuilderVisitor::visitMultiplicationExpression(kyoto::KyotoParser::MultiplicationExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new MulNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitDivisionExpression(kyoto::KyotoParser::DivisionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new DivNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitModulusExpression(kyoto::KyotoParser::ModulusExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new ModNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitAdditionExpression(kyoto::KyotoParser::AdditionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new AddNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitSubtractionExpression(kyoto::KyotoParser::SubtractionExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new SubNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitLessThanExpression(kyoto::KyotoParser::LessThanExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new LessNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitGreaterThanExpression(kyoto::KyotoParser::GreaterThanExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new GreaterNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitLessThanOrEqualExpression(kyoto::KyotoParser::LessThanOrEqualExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new LessEqNode(lhs, rhs, compiler);
}

std::any
ASTBuilderVisitor::visitGreaterThanOrEqualExpression(kyoto::KyotoParser::GreaterThanOrEqualExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new GreaterEqNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitEqualsExpression(kyoto::KyotoParser::EqualsExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new EqNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitNotEqualsExpression(kyoto::KyotoParser::NotEqualsExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new NotEqNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitLogicalAndExpression(kyoto::KyotoParser::LogicalAndExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new LogicalAndNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitLogicalOrExpression(kyoto::KyotoParser::LogicalOrExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->children[0]));
    auto* rhs = std::any_cast<ExpressionNode*>(visit(ctx->children[2]));
    return (ExpressionNode*)new LogicalOrNode(lhs, rhs, compiler);
}

std::any ASTBuilderVisitor::visitParenthesizedExpression(kyoto::KyotoParser::ParenthesizedExpressionContext* ctx)
{
    return visit(ctx->expression());
}

std::any ASTBuilderVisitor::visitIfStatement(kyoto::KyotoParser::IfStatementContext* ctx)
{
    auto* condition = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    auto* then_node = std::any_cast<ASTNode*>(visit(ctx->block(0)));
    ASTNode* else_node = nullptr;

    if (ctx->ELSE()) {
        else_node = ctx->ifStatement() ? std::any_cast<ASTNode*>(visit(ctx->ifStatement()))
                                       : std::any_cast<ASTNode*>(visit(ctx->block(1)));
    }

    return (ASTNode*)new IfStatementNode(condition, then_node, else_node, compiler);
}

PrimitiveType::Kind ASTBuilderVisitor::parse_primitive_type(const std::string& type) const
{
    if (type == "bool") return PrimitiveType::Kind::Boolean;
    if (type == "char") return PrimitiveType::Kind::Char;
    if (type == "i8") return PrimitiveType::Kind::I8;
    if (type == "i16") return PrimitiveType::Kind::I16;
    if (type == "i32") return PrimitiveType::Kind::I32;
    if (type == "i64") return PrimitiveType::Kind::I64;
    if (type == "f32") return PrimitiveType::Kind::F32;
    if (type == "f64") return PrimitiveType::Kind::F64;
    if (type == "string") return PrimitiveType::Kind::String;
    if (type == "void") return PrimitiveType::Kind::Void;
    return PrimitiveType::Kind::Unknown;
}

std::optional<int64_t> ASTBuilderVisitor::parse_signed_integer_into(const std::string& str,
                                                                    const PrimitiveType::Kind kind) const
{
    try {
        switch (kind) {
        case PrimitiveType::Kind::I8:
        case PrimitiveType::Kind::I16:
        case PrimitiveType::Kind::I32:
        case PrimitiveType::Kind::I64: {
            auto num = std::stoll(str);
            if (compiler.get_type_resolver().fits_in(num, kind)) return num;
            return std::nullopt;
        }
        case PrimitiveType::Kind::Boolean: {
            if (str == "true") return 1;
            if (str == "false") return 0;
            return std::nullopt;
        }
        default:
            return std::nullopt;
        }
    } catch (std::invalid_argument&) {
        return std::nullopt;
    }
}

std::optional<double> ASTBuilderVisitor::parse_double(const std::string& str) const
{
    try {
        return std::stod(str);
    } catch (std::invalid_argument&) {
        return std::nullopt;
    }
}

std::optional<float> ASTBuilderVisitor::parse_float(const std::string& str) const
{
    try {
        return std::stof(str);
    } catch (std::invalid_argument&) {
        return std::nullopt;
    }
}

std::optional<bool> ASTBuilderVisitor::parse_bool(const std::string& str) const
{
    if (str == "true") return true;
    if (str == "false") return false;
    return std::nullopt;
}