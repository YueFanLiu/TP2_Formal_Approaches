/* TP2 adaptation of Mouzheng LI and Yuefan LIU's TP1 scanner.
   Matching rules are preserved; actions now store tokens instead of printing. */
%option c++ yyclass="TokenScanner" noyywrap nodefault yylineno nounput noinput

%{
#include "scanner.hpp"
#include <stdexcept>
%}

DIGIT       [0-9]
LETTER      [A-Za-z]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*
NUMBER      {DIGIT}+
STRING      \"[^\"\r\n]*\"
WHITESPACE  ([ \t]|\r?\n)+

%%

"int"       { add(TokenType::Int, yytext); }
"if"        { add(TokenType::If, yytext); }
"else"      { add(TokenType::Else, yytext); }
"while"     { add(TokenType::While, yytext); }
"for"       { add(TokenType::For, yytext); }
"switch"    { add(TokenType::Switch, yytext); }
"case"      { add(TokenType::Case, yytext); }
"default"   { add(TokenType::Default, yytext); }
"break"     { add(TokenType::Break, yytext); }
"print"     { add(TokenType::Print, yytext); }

{STRING}    { add(TokenType::String, yytext); }
{NUMBER}    { add(TokenType::Number, yytext); }
{IDENTIFIER} { add(TokenType::Identifier, yytext); }

">="|"<="|"=="|"!="|">"|"<" { add(TokenType::Comparison, yytext); }
"="         { add(TokenType::Assign, yytext); }
"+"|"-"|"*"|"/" { add(TokenType::Arithmetic, yytext); }

"("         { add(TokenType::LeftParen, yytext); }
")"         { add(TokenType::RightParen, yytext); }
"{"         { add(TokenType::LeftBrace, yytext); }
"}"         { add(TokenType::RightBrace, yytext); }
";"         { add(TokenType::Semicolon, yytext); }
","         { add(TokenType::Comma, yytext); }
":"         { add(TokenType::Colon, yytext); }

"//"[^\r\n]* { /* Skip line comments, as in TP1. */ }
{WHITESPACE} { /* Skip whitespace. */ }

\"[^\"\r\n]* {
    throw std::runtime_error("Line " + std::to_string(lineno()) +
                             ": string has no closing quote");
}
. {
    throw std::runtime_error("Line " + std::to_string(lineno()) +
                             ": unknown character '" + yytext + "'");
}

%%

std::vector<Token> scan_tokens(std::istream& input) {
    std::vector<Token> tokens;
    TokenScanner scanner(input, tokens);
    scanner.yylex();
    tokens.push_back(Token{TokenType::End, "<EOF>", scanner.lineno()});
    return tokens;
}
