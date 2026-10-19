#include "parser.hpp"
#include <stdexcept>
#include <utility>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    if (tokens_.empty() || tokens_.back().type != TokenType::End)
        tokens_.push_back({TokenType::End, "<EOF>",
                           tokens_.empty() ? 1 : tokens_.back().line});
}

const Token& Parser::current() const {
    return tokens_.at(pos_);
}

[[noreturn]] void Parser::error(const std::string& message) const {
    throw std::runtime_error("Line " + std::to_string(current().line) + ": " +
                             message + "; got '" + current().value + "'");
}

Token Parser::consume(TokenType type, const std::string& expected) {
    if (current().type != type)
        error("expected " + expected);
    Token token = current();
    if (type != TokenType::End)
        ++pos_;
    return token;
}

std::shared_ptr<Program> Parser::get_root() {
    auto program = std::make_shared<Program>();
    while (current().type != TokenType::End)
        program->statements.push_back(statement());
    return program;
}

NodePtr Parser::statement() {
    switch (current().type) {
    case TokenType::Int: return declarations();
    case TokenType::Identifier: return assignment();
    case TokenType::Print: return printing();
    case TokenType::If: return if_statement();
    case TokenType::While: return while_statement();
    case TokenType::For: return for_statement();
    case TokenType::Switch: return switch_statement();
    default:
        error("expected a statement");
    }
}

// Declaration = "int" IDENTIFIER { "," IDENTIFIER } ";" ;
NodePtr Parser::declarations() {
    consume(TokenType::Int, "'int'");
    auto declaration = std::make_shared<Declaration>();
    declaration->identifiers.push_back(consume(TokenType::Identifier, "identifier"));
    while (current().type == TokenType::Comma) {
        consume(TokenType::Comma, "','");
        declaration->identifiers.push_back(consume(TokenType::Identifier, "identifier"));
    }
    consume(TokenType::Semicolon, "';'");
    return declaration;
}

// Assignment = IDENTIFIER "=" Expression ";" ;
NodePtr Parser::assignment() {
    auto result = assignment_core();
    consume(TokenType::Semicolon, "';'");
    return result;
}

// The for header owns its separators, so this rule has no final semicolon.
NodePtr Parser::assignment_core() {
    Token id = consume(TokenType::Identifier, "identifier");
    consume(TokenType::Assign, "'='");
    auto expression = expr();
    return std::make_shared<Assignment>(std::move(id), std::move(expression));
}

NodePtr Parser::print_argument() {
    if (current().type == TokenType::Identifier)
        return std::make_shared<IdentifierNode>(consume(TokenType::Identifier, "identifier"));
    if (current().type == TokenType::String)
        return std::make_shared<StringNode>(consume(TokenType::String, "string"));
    error("expected a variable or string in print");
}

NodePtr Parser::printing() {
    consume(TokenType::Print, "'print'");
    consume(TokenType::LeftParen, "'('");
    auto result = std::make_shared<PrintStatement>();
    result->arguments.push_back(print_argument());
    while (current().type == TokenType::Comma) {
        consume(TokenType::Comma, "','");
        result->arguments.push_back(print_argument());
    }
    consume(TokenType::RightParen, "')'");
    consume(TokenType::Semicolon, "';'");
    return result;
}

// Condition = Expression CompareOperator Expression ;
std::shared_ptr<Condition> Parser::condition() {
    auto left = expr();
    Token op = consume(TokenType::Comparison, "comparison operator");
    auto right = expr();
    return std::make_shared<Condition>(std::move(left), std::move(op), std::move(right));
}

std::shared_ptr<Block> Parser::block() {
    consume(TokenType::LeftBrace, "'{'");
    auto result = std::make_shared<Block>();
    while (current().type != TokenType::RightBrace) {
        if (current().type == TokenType::End)
            error("expected '}' to close block");
        result->statements.push_back(statement());
    }
    consume(TokenType::RightBrace, "'}'");
    return result;
}

NodePtr Parser::if_statement() {
    consume(TokenType::If, "'if'");
    consume(TokenType::LeftParen, "'('");
    auto result = std::make_shared<IfStatement>();
    result->condition = condition();
    consume(TokenType::RightParen, "')'");
    result->then_body = block();
    if (current().type == TokenType::Else) {
        consume(TokenType::Else, "'else'");
        result->else_body = block();
    }
    return result;
}

NodePtr Parser::while_statement() {
    consume(TokenType::While, "'while'");
    consume(TokenType::LeftParen, "'('");
    auto result = std::make_shared<WhileStatement>();
    result->condition = condition();
    consume(TokenType::RightParen, "')'");
    result->body = block();
    return result;
}

NodePtr Parser::for_statement() {
    consume(TokenType::For, "'for'");
    consume(TokenType::LeftParen, "'('");
    auto result = std::make_shared<ForStatement>();
    if (current().type != TokenType::Semicolon)
        result->initialization = assignment_core();
    consume(TokenType::Semicolon, "';' after for initialization");
    if (current().type != TokenType::Semicolon)
        result->condition = condition();
    consume(TokenType::Semicolon, "';' after for condition");
    if (current().type != TokenType::RightParen)
        result->update = assignment_core();
    consume(TokenType::RightParen, "')'");
    result->body = block();
    return result;
}

// Each branch ends at case/default/}, or at its optional trailing break.
std::shared_ptr<SwitchBranch> Parser::switch_branch() {
    auto branch = std::make_shared<SwitchBranch>();
    if (current().type == TokenType::Case) {
        consume(TokenType::Case, "'case'");
        branch->value = consume(TokenType::Number, "case number");
    } else {
        consume(TokenType::Default, "'default'");
    }
    consume(TokenType::Colon, "':'");
    branch->body = std::make_shared<Block>();
    while (current().type != TokenType::Case &&
           current().type != TokenType::Default &&
           current().type != TokenType::RightBrace &&
           current().type != TokenType::Break) {
        if (current().type == TokenType::End)
            error("expected '}' to close switch");
        branch->body->statements.push_back(statement());
    }
    if (current().type == TokenType::Break) {
        consume(TokenType::Break, "'break'");
        consume(TokenType::Semicolon, "';' after break");
        branch->body->statements.push_back(std::make_shared<BreakStatement>());
        if (current().type != TokenType::Case &&
            current().type != TokenType::Default &&
            current().type != TokenType::RightBrace)
            error("expected case, default or '}' after break");
    }
    return branch;
}

NodePtr Parser::switch_statement() {
    consume(TokenType::Switch, "'switch'");
    consume(TokenType::LeftParen, "'('");
    auto result = std::make_shared<SwitchStatement>(
        consume(TokenType::Identifier, "switch variable"));
    consume(TokenType::RightParen, "')'");
    consume(TokenType::LeftBrace, "'{'");
    while (current().type == TokenType::Case)
        result->branches.push_back(switch_branch());
    if (current().type == TokenType::Default)
        result->branches.push_back(switch_branch());
    consume(TokenType::RightBrace, "'}' (default must be the final branch)");
    return result;
}

// Expression = Term { ("+" | "-") Term } ;
NodePtr Parser::expr() {
    auto left = term();
    while (current().value == "+" || current().value == "-") {
        Token op = consume(TokenType::Arithmetic, "'+' or '-'");
        auto right = term();
        left = std::make_shared<BinOpNode>(left, std::move(op), std::move(right));
    }
    return left;
}

// Term = Factor { ("*" | "/") Factor } ;
NodePtr Parser::term() {
    auto left = factor();
    while (current().value == "*" || current().value == "/") {
        Token op = consume(TokenType::Arithmetic, "'*' or '/'");
        auto right = factor();
        left = std::make_shared<BinOpNode>(left, std::move(op), std::move(right));
    }
    return left;
}

// Factor = IDENTIFIER | Number | "(" Expression ")" ;
NodePtr Parser::factor() {
    if (current().type == TokenType::Number)
        return std::make_shared<NumberNode>(consume(TokenType::Number, "number"));
    if (current().type == TokenType::Identifier)
        return std::make_shared<IdentifierNode>(consume(TokenType::Identifier, "identifier"));
    if (current().type == TokenType::LeftParen) {
        consume(TokenType::LeftParen, "'('");
        auto expression = expr();
        consume(TokenType::RightParen, "')'");
        return expression;
    }
    error("expected a number, identifier or '('");
}
