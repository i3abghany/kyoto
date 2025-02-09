#include <any>
#include <assert.h>
#include <initializer_list>
#include <optional>
#include <regex>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include "KyotoParser.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/ClassDefinitionNode.h"
#include "kyoto/AST/DeclarationNodes.h"
#include "kyoto/AST/Expressions/ArrayNode.h"
#include "kyoto/AST/Expressions/AssignmentNode.h"
#include "kyoto/AST/Expressions/BinaryNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"
#include "kyoto/AST/Expressions/FunctionCallNode.h"
#include "kyoto/AST/Expressions/IdentifierNode.h"
#include "kyoto/AST/Expressions/MemberAccessNode.h"
#include "kyoto/AST/Expressions/MethodCallNode.h"
#include "kyoto/AST/Expressions/NumberNode.h"
#include "kyoto/AST/Expressions/StringLiteralNode.h"
#include "kyoto/AST/Expressions/UnaryNode.h"
#include "kyoto/AST/ForStatementNode.h"
#include "kyoto/AST/IfStatementNode.h"
#include "kyoto/AST/ReturnStatement.h"
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
    for (const auto node : ctx->topLevel()) {
        std::any result = visit(node);
        nodes.push_back(std::any_cast<ASTNode*>(result));
    }
    return (ASTNode*)new ProgramNode(nodes, compiler);
}

std::any ASTBuilderVisitor::visitCdecl(kyoto::KyotoParser::CdeclContext* ctx)
{
    const auto name = ctx->IDENTIFIER()->getText();
    std::vector<FunctionNode::Parameter> args;
    for (const auto& paramCtx : ctx->parameterList()->parameter()) {
        auto* type = std::any_cast<KType*>(visit(paramCtx->type()));
        args.push_back({ paramCtx->IDENTIFIER()->getText(), type });
    }

    auto* ret_type = std::any_cast<KType*>(visit(ctx->type()));
    const auto varargs = ctx->parameterList()->ELLIPSIS() != nullptr;
    return (ASTNode*)new FunctionNode(name, args, varargs, ret_type, nullptr, compiler);
}

std::any ASTBuilderVisitor::visitFunctionDefinition(kyoto::KyotoParser::FunctionDefinitionContext* ctx)
{
    auto name = ctx->IDENTIFIER()->getText();
    if (compiler.get_current_class() != "") name = compiler.get_current_class() + "_" + name;

    std::vector<FunctionNode::Parameter> args;

    for (const auto param_ctx : ctx->parameterList()->parameter()) {
        auto* type = std::any_cast<KType*>(visit(param_ctx->type()));
        args.push_back({ param_ctx->IDENTIFIER()->getText(), type });
    }

    auto* ret_type = ctx->type() ? std::any_cast<KType*>(visit(ctx->type())) : KType::get_void();
    ASTNode* body = nullptr;
    try {
        body = std::any_cast<ASTNode*>(visit(ctx->block()));
    } catch (const std::bad_any_cast&) {
        delete ret_type;
        throw;
    }
    const auto varargs = ctx->parameterList()->ELLIPSIS() != nullptr;
    return (ASTNode*)new FunctionNode(name, args, varargs, ret_type, body, compiler);
}

std::any ASTBuilderVisitor::visitBlock(kyoto::KyotoParser::BlockContext* ctx)
{
    std::vector<ASTNode*> nodes;
    for (const auto& stmt : ctx->statement()) {
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
    auto* type = std::any_cast<KType*>(visit(ctx->type()));
    std::string name = ctx->IDENTIFIER()->getText();
    return (ASTNode*)new DeclarationStatementNode(name, type, compiler);
}

std::any ASTBuilderVisitor::visitRegularDeclaration(kyoto::KyotoParser::RegularDeclarationContext* ctx)
{
    auto* type = std::any_cast<KType*>(visit(ctx->type()));
    std::string name = ctx->IDENTIFIER()->getText();
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    if (type->is_array() && expr->get_ktype()->is_array())
        type->as<ArrayType>()->set_size(expr->get_ktype()->as<ArrayType>()->get_size());
    return (ASTNode*)new FullDeclarationStatementNode(name, type, expr, compiler);
}

std::any ASTBuilderVisitor::visitTypelessDeclaration(kyoto::KyotoParser::TypelessDeclarationContext* ctx)
{
    std::string name = ctx->IDENTIFIER()->getText();
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ASTNode*)new FullDeclarationStatementNode(name, nullptr, expr, compiler);
}

std::any ASTBuilderVisitor::visitAssignmentExpression(kyoto::KyotoParser::AssignmentExpressionContext* ctx)
{
    const auto assignee = std::any_cast<ExpressionNode*>(visit(ctx->expression(0)));
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression(1)));
    return (ExpressionNode*)new AssignmentNode(assignee, expr, compiler);
}

std::any ASTBuilderVisitor::visitReturnStatement(kyoto::KyotoParser::ReturnStatementContext* ctx)
{
    auto* expr = ctx->expression() ? std::any_cast<ExpressionNode*>(visit(ctx->expression())) : nullptr;
    return (ASTNode*)new ReturnStatementNode(expr, compiler);
}

std::any ASTBuilderVisitor::visitFunctionCallExpression(kyoto::KyotoParser::FunctionCallExpressionContext* ctx)
{
    const auto name = ctx->IDENTIFIER()->getText();
    std::vector<ExpressionNode*> args;
    for (const auto arg : ctx->expressionList()->expression()) {
        args.push_back(std::any_cast<ExpressionNode*>(visit(arg)));
    }
    return (ExpressionNode*)new FunctionCall(name, args, compiler);
}

std::any ASTBuilderVisitor::visitStringExpression(kyoto::KyotoParser::StringExpressionContext* ctx)
{
    const auto txt = std::regex_replace(ctx->getText(), std::regex(R"(\\n)"), "\n");
    return (ExpressionNode*)new StringLiteralNode(txt.substr(1, txt.size() - 2), compiler);
}

std::any ASTBuilderVisitor::visitCharExpression(kyoto::KyotoParser::CharExpressionContext* ctx)
{
    const auto txt = ctx->getText();
    return (ExpressionNode*)new NumberNode(txt[1], new PrimitiveType(PrimitiveType::Kind::Char), compiler);
}

std::any ASTBuilderVisitor::visitNumberExpression(kyoto::KyotoParser::NumberExpressionContext* ctx)
{
    const auto txt = ctx->getText();

    // The resulting node will have the smallest possible integer type.
    // Terminate if we can't parse the number.

    using Kind = PrimitiveType::Kind;
    for (const auto kind : { Kind::I32, Kind::I64, Kind::Boolean }) {
        if (auto num = parse_signed_integer_into(txt, kind); num.has_value())
            return (ExpressionNode*)new NumberNode(num.value(), new PrimitiveType(kind), compiler);
    }

    assert(false && "Unknown Integer type");
}

std::any ASTBuilderVisitor::visitIdentifierExpression(kyoto::KyotoParser::IdentifierExpressionContext* ctx)
{
    return (ExpressionNode*)new IdentifierExpressionNode(ctx->IDENTIFIER()->getText(), compiler);
}

std::any ASTBuilderVisitor::visitAddressOfExpression(kyoto::KyotoParser::AddressOfExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::AddressOf, compiler);
}

std::any ASTBuilderVisitor::visitDereferenceExpression(kyoto::KyotoParser::DereferenceExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::Dereference, compiler);
}

std::any ASTBuilderVisitor::visitMemberAccessExpression(kyoto::KyotoParser::MemberAccessExpressionContext* ctx)
{
    auto* lhs = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    const auto member = ctx->IDENTIFIER()->getText();
    return (ExpressionNode*)new MemberAccessNode(lhs, member, compiler);
}

std::any ASTBuilderVisitor::visitMethodCallExpression(kyoto::KyotoParser::MethodCallExpressionContext* ctx)
{
    const auto instance_name = ctx->expression()->getText();
    const auto name = ctx->IDENTIFIER()->getText();
    std::vector<ExpressionNode*> args;
    for (const auto arg : ctx->expressionList()->expression()) {
        args.push_back(std::any_cast<ExpressionNode*>(visit(arg)));
    }
    return (ExpressionNode*)new MethodCall(instance_name, name, args, compiler);
}

std::any ASTBuilderVisitor::visitPrefixIncrementExpression(kyoto::KyotoParser::PrefixIncrementExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::PrefixIncrement, compiler);
}

std::any ASTBuilderVisitor::visitPrefixDecrementExpression(kyoto::KyotoParser::PrefixDecrementExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::PrefixDecrement, compiler);
}

std::any ASTBuilderVisitor::visitNegationExpression(kyoto::KyotoParser::NegationExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::Negate, compiler);
}

std::any ASTBuilderVisitor::visitPositiveExpression(kyoto::KyotoParser::PositiveExpressionContext* ctx)
{
    auto* expr = std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return (ExpressionNode*)new UnaryNode(expr, UnaryNode::UnaryOp::Positive, compiler);
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

std::any ASTBuilderVisitor::visitArrayExpression(kyoto::KyotoParser::ArrayExpressionContext* ctx)
{
    auto elem_type = std::any_cast<KType*>(visit(ctx->type()));
    auto* type = new ArrayType(elem_type, ctx->expressionList()->expression().size());
    std::vector<ExpressionNode*> elems;
    for (const auto arg : ctx->expressionList()->expression()) {
        elems.push_back(std::any_cast<ExpressionNode*>(visit(arg)));
    }
    return (ExpressionNode*)new ArrayNode(elems, type, compiler);
}

std::any ASTBuilderVisitor::visitIfStatement(kyoto::KyotoParser::IfStatementContext* ctx)
{
    std::vector<ExpressionNode*> conditions;
    std::vector<ASTNode*> bodies;

    conditions.push_back(std::any_cast<ExpressionNode*>(visit(ctx->expression())));
    bodies.push_back(std::any_cast<ASTNode*>(visit(ctx->block())));

    kyoto::KyotoParser::ElseIfElseStatementContext* else_if_else = ctx->elseIfElseStatement();

    while (true) {

        if (auto* else_if = dynamic_cast<kyoto::KyotoParser::ElseIfStatementContext*>(else_if_else); else_if) {
            conditions.push_back(std::any_cast<ExpressionNode*>(visit(else_if->expression())));
            bodies.push_back(std::any_cast<ASTNode*>(visit(else_if->block())));
            else_if_else = else_if->elseIfElseStatement();
            continue;
        }

        if (auto* else_stmt = dynamic_cast<kyoto::KyotoParser::ElseStatementContext*>(else_if_else); else_stmt) {
            if (auto* block = else_stmt->optionalElseStatement()->block(); block)
                bodies.push_back(std::any_cast<ASTNode*>(visit(block)));
            break;
        }
    }

    return (ASTNode*)new IfStatementNode(conditions, bodies, compiler);
}

std::any ASTBuilderVisitor::visitForInit(kyoto::KyotoParser::ForInitContext* ctx)
{
    if (ctx->fullDeclaration()) return visit(ctx->fullDeclaration());
    if (ctx->expressionStatement()) return visit(ctx->expressionStatement());
    return {};
}

std::any ASTBuilderVisitor::visitForCondition(kyoto::KyotoParser::ForConditionContext* ctx)
{
    if (ctx->expressionStatement())
        return (ExpressionStatementNode*)std::any_cast<ASTNode*>(visit(ctx->expressionStatement()));
    return {};
}

std::any ASTBuilderVisitor::visitForUpdate(kyoto::KyotoParser::ForUpdateContext* ctx)
{
    if (ctx->expression()) return std::any_cast<ExpressionNode*>(visit(ctx->expression()));
    return {};
}

std::any ASTBuilderVisitor::visitForStatement(kyoto::KyotoParser::ForStatementContext* ctx)
{

    auto init_any = visit(ctx->forInit());
    auto* init = init_any.has_value() ? std::any_cast<ASTNode*>(init_any) : nullptr;

    auto condition_any = visit(ctx->forCondition());
    auto* condition = condition_any.has_value() ? std::any_cast<ExpressionStatementNode*>(condition_any) : nullptr;

    auto update_any = visit(ctx->forUpdate());
    auto* update = update_any.has_value() ? std::any_cast<ExpressionNode*>(update_any) : nullptr;

    auto* body = std::any_cast<ASTNode*>(visit(ctx->block()));
    return (ASTNode*)new ForStatementNode(init, condition, update, body, compiler);
}

std::any ASTBuilderVisitor::visitClassDefinition(kyoto::KyotoParser::ClassDefinitionContext* ctx)
{
    std::vector<ASTNode*> components;
    compiler.push_class(ctx->IDENTIFIER(0)->getText());
    for (const auto& component : ctx->classComponents()->classComponent()) {
        components.push_back(std::any_cast<ASTNode*>(visit(component)));
    }
    compiler.pop_class();
    std::string parent = ctx->IDENTIFIER().size() > 1 ? ctx->IDENTIFIER(1)->getText() : "";
    return (ASTNode*)new ClassDefinitionNode(ctx->IDENTIFIER(0)->getText(), parent, components, compiler);
}

std::any ASTBuilderVisitor::visitClassComponent(kyoto::KyotoParser::ClassComponentContext* ctx)
{
    if (ctx->functionDefinition()) return visit(ctx->functionDefinition());
    if (ctx->declaration()) return visit(ctx->declaration());
    if (ctx->constructorDefinition()) return visit(ctx->constructorDefinition());
    return nullptr;
}

std::any ASTBuilderVisitor::visitConstructorDefinition(kyoto::KyotoParser::ConstructorDefinitionContext* ctx)
{
    std::vector<FunctionNode::Parameter> args;
    for (const auto param_ctx : ctx->parameterList()->parameter()) {
        auto* type = std::any_cast<KType*>(visit(param_ctx->type()));
        args.push_back({ param_ctx->IDENTIFIER()->getText(), type });
    }

    auto* body = std::any_cast<ASTNode*>(visit(ctx->block()));
    auto name = compiler.get_current_class() + "_" + "constructor";
    return (ASTNode*)new ConstructorNode(name, args, body, compiler);
}

std::any ASTBuilderVisitor::visitBoolType(kyoto::KyotoParser::BoolTypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::Boolean);
}

std::any ASTBuilderVisitor::visitCharType(kyoto::KyotoParser::CharTypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::Char);
}

std::any ASTBuilderVisitor::visitI8Type(kyoto::KyotoParser::I8TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::I8);
}

std::any ASTBuilderVisitor::visitI16Type(kyoto::KyotoParser::I16TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::I16);
}

std::any ASTBuilderVisitor::visitI32Type(kyoto::KyotoParser::I32TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::I32);
}

std::any ASTBuilderVisitor::visitI64Type(kyoto::KyotoParser::I64TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::I64);
}

std::any ASTBuilderVisitor::visitF32Type(kyoto::KyotoParser::F32TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::F32);
}

std::any ASTBuilderVisitor::visitF64Type(kyoto::KyotoParser::F64TypeContext* ctx)
{
    return (KType*)new PrimitiveType(PrimitiveType::Kind::F64);
}

std::any ASTBuilderVisitor::visitVoidType(kyoto::KyotoParser::VoidTypeContext* ctx)
{
    return (KType*)KType::get_void();
}

std::any ASTBuilderVisitor::visitStrType(kyoto::KyotoParser::StrTypeContext* ctx)
{
    return (KType*)new PointerType(new PrimitiveType(PrimitiveType::Kind::Char));
}

std::any ASTBuilderVisitor::visitArrayType(kyoto::KyotoParser::ArrayTypeContext* ctx)
{
    auto* type = std::any_cast<KType*>(visit(ctx->type()));
    return (KType*)new ArrayType(type, 0);
}

std::any ASTBuilderVisitor::visitPointerType(kyoto::KyotoParser::PointerTypeContext* ctx)
{
    auto* type = std::any_cast<KType*>(visit(ctx->type()));
    for (size_t i = 0; i < ctx->pointerSuffix().size(); i++)
        type = new PointerType(type);
    return type;
}

std::any ASTBuilderVisitor::visitClassType(kyoto::KyotoParser::ClassTypeContext* ctx)
{
    return (KType*)new ClassType(ctx->IDENTIFIER()->getText());
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
        case PrimitiveType::Kind::Boolean:
            return parse_bool(str);
        default:
            return std::nullopt;
        }
    } catch (std::invalid_argument&) {
        return std::nullopt;
    }
}

std::optional<int64_t> ASTBuilderVisitor::parse_bool(const std::string& str) const
{
    if (str == "true") return 1;
    if (str == "false") return 0;
    return std::nullopt;
}
