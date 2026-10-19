#pragma once
#include <cstddef>
#include <vector>
#include "ast.hpp"
#include "tokens.hpp"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::shared_ptr<Program> get_root();

private:
    const Token& current() const;
    Token consume(TokenType type, const std::string& expected);
    [[noreturn]] void error(const std::string& message) const;
    NodePtr statement();
    NodePtr declarations();
    NodePtr assignment();
    NodePtr assignment_core();
    NodePtr printing();
    NodePtr print_argument();
    std::shared_ptr<Condition> condition();
    std::shared_ptr<Block> block();
    NodePtr if_statement();
    NodePtr while_statement();
    NodePtr for_statement();
    NodePtr switch_statement();
    std::shared_ptr<SwitchBranch> switch_branch();
    NodePtr expr();
    NodePtr term();
    NodePtr factor();

    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
};
