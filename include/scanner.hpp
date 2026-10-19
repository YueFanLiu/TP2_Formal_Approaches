#pragma once
#include <istream>
#include <vector>
#include "tokens.hpp"

// Flex's generated source already includes FlexLexer.h before this header.
#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

class TokenScanner : public yyFlexLexer {
public:
    TokenScanner(std::istream& input, std::vector<Token>& tokens)
        : yyFlexLexer(&input), tokens_(tokens) {}
    int yylex() override;

private:
    void add(TokenType type, const char* text) {
        tokens_.push_back(Token{type, text, lineno()});
    }
    std::vector<Token>& tokens_;
};

std::vector<Token> scan_tokens(std::istream& input);
