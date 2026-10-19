#pragma once
#include <string>

// Keep all tokens from our TP1 grammar, including statements added in later steps.
enum class TokenType {
    Int, If, Else, While, For, Switch, Case, Default, Break, Print,
    Identifier, Number, String,
    Comparison, Assign, Arithmetic,
    LeftParen, RightParen, LeftBrace, RightBrace, Semicolon, Comma, Colon,
    End
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};
