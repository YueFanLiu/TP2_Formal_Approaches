#pragma once
#include <memory>
#include <ostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "tokens.hpp"

struct Node {
    virtual ~Node() = default;
    virtual void print(std::ostream& out, int depth) const = 0;
};
using NodePtr = std::shared_ptr<Node>;

struct Program : Node {
    std::vector<NodePtr> statements;
    void print(std::ostream& out, int depth) const override;
};

struct NumberNode : Node {
    Token token;
    explicit NumberNode(Token t) : token(std::move(t)) {}
    void print(std::ostream& out, int depth) const override;
};

struct IdentifierNode : Node {
    Token token;
    explicit IdentifierNode(Token t) : token(std::move(t)) {}
    void print(std::ostream& out, int depth) const override;
};

struct BinOpNode : Node {
    NodePtr left;
    Token op;
    NodePtr right;
    BinOpNode(NodePtr l, Token o, NodePtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    void print(std::ostream& out, int depth) const override;
};

struct Declaration : Node {
    std::vector<Token> identifiers;
    void print(std::ostream& out, int depth) const override;
};

struct Assignment : Node {
    Token identifier;
    NodePtr expression;
    Assignment(Token id, NodePtr expr)
        : identifier(std::move(id)), expression(std::move(expr)) {}
    void print(std::ostream& out, int depth) const override;
};

struct StringNode : Node {
    Token token;
    explicit StringNode(Token t) : token(std::move(t)) {}
    void print(std::ostream& out, int depth) const override;
};

struct Block : Node {
    std::vector<NodePtr> statements;
    void print(std::ostream& out, int depth) const override;
};

struct Condition : Node {
    NodePtr left;
    Token op;
    NodePtr right;
    Condition(NodePtr l, Token o, NodePtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    void print(std::ostream& out, int depth) const override;
};

struct PrintStatement : Node {
    std::vector<NodePtr> arguments;
    void print(std::ostream& out, int depth) const override;
};

struct IfStatement : Node {
    std::shared_ptr<Condition> condition;
    std::shared_ptr<Block> then_body;
    std::shared_ptr<Block> else_body; // null means no else clause
    void print(std::ostream& out, int depth) const override;
};

struct WhileStatement : Node {
    std::shared_ptr<Condition> condition;
    std::shared_ptr<Block> body;
    void print(std::ostream& out, int depth) const override;
};

struct ForStatement : Node {
    NodePtr initialization; // header components may be absent
    std::shared_ptr<Condition> condition;
    NodePtr update;
    std::shared_ptr<Block> body;
    void print(std::ostream& out, int depth) const override;
};

struct BreakStatement : Node {
    void print(std::ostream& out, int depth) const override;
};

struct SwitchBranch : Node {
    std::optional<Token> value; // no value means default
    std::shared_ptr<Block> body;
    void print(std::ostream& out, int depth) const override;
};

struct SwitchStatement : Node {
    Token identifier;
    std::vector<std::shared_ptr<SwitchBranch>> branches;
    explicit SwitchStatement(Token id) : identifier(std::move(id)) {}
    void print(std::ostream& out, int depth) const override;
};

void print_ast(const Node& root, std::ostream& out);
