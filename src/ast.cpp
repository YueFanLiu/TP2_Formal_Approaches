#include "ast.hpp"

static void print_line(std::ostream& out, int depth, const std::string& text) {
    out << std::string(depth * 2, ' ') << text << '\n';
}

// Preorder DFS: print this node, then recursively print each child in order.
void Program::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Program");
    for (const auto& statement : statements)
        statement->print(out, depth + 1);
}

void NumberNode::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Number: " + token.value);
}

void IdentifierNode::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Identifier: " + token.value);
}

void BinOpNode::print(std::ostream& out, int depth) const {
    print_line(out, depth, "BinaryOp: " + op.value);
    left->print(out, depth + 1);
    right->print(out, depth + 1);
}

void Declaration::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Declaration: int");
    for (const auto& id : identifiers)
        IdentifierNode(id).print(out, depth + 1);
}

void Assignment::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Assignment");
    IdentifierNode(identifier).print(out, depth + 1);
    expression->print(out, depth + 1);
}

void StringNode::print(std::ostream& out, int depth) const {
    // Keep the quotes and raw contents from TP1; do not interpret escapes.
    print_line(out, depth, "String: " + token.value);
}

void Block::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Block");
    for (const auto& statement : statements)
        statement->print(out, depth + 1);
}

void Condition::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Condition: " + op.value);
    left->print(out, depth + 1);
    right->print(out, depth + 1);
}

void PrintStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Print");
    for (const auto& argument : arguments)
        argument->print(out, depth + 1);
}

void IfStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "If");
    condition->print(out, depth + 1);
    print_line(out, depth + 1, "Then");
    then_body->print(out, depth + 2);
    if (else_body) {
        print_line(out, depth + 1, "Else");
        else_body->print(out, depth + 2);
    }
}

void WhileStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "While");
    condition->print(out, depth + 1);
    body->print(out, depth + 1);
}

static void print_slot(std::ostream& out, int depth, const std::string& name,
                       const NodePtr& child) {
    print_line(out, depth, name);
    if (child)
        child->print(out, depth + 1);
    else
        print_line(out, depth + 1, "<empty>");
}

void ForStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "For");
    print_slot(out, depth + 1, "Initialization", initialization);
    print_slot(out, depth + 1, "Condition", condition);
    print_slot(out, depth + 1, "Update", update);
    body->print(out, depth + 1);
}

void BreakStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Break");
}

void SwitchBranch::print(std::ostream& out, int depth) const {
    print_line(out, depth, value ? "Case: " + value->value : "Default");
    body->print(out, depth + 1);
}

void SwitchStatement::print(std::ostream& out, int depth) const {
    print_line(out, depth, "Switch");
    IdentifierNode(identifier).print(out, depth + 1);
    for (const auto& branch : branches)
        branch->print(out, depth + 1);
}

void print_ast(const Node& root, std::ostream& out) {
    root.print(out, 0);
}
