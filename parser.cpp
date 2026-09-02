#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

// This parser works with the Token and TokenType definitions from lexer.cpp.
// In the final project, move those definitions into lexer.h and include that
// header here.
//
// For now, parser.cpp includes lexer.cpp so it can be tested directly.
// Remove the include below once a proper lexer.h is created.
#include "lexer.cpp"

using namespace std;

// ============================================================
// AST NODE DEFINITIONS
// ============================================================

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;
};

using ASTPtr = shared_ptr<ASTNode>;

struct NumberNode : ASTNode {
    string value;

    explicit NumberNode(const string& value) : value(value) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "Number(" << value << ")\n";
    }
};

struct IdentifierNode : ASTNode {
    string name;

    explicit IdentifierNode(const string& name) : name(name) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "Identifier(" << name << ")\n";
    }
};

struct BinaryOpNode : ASTNode {
    string op;
    ASTPtr left;
    ASTPtr right;

    BinaryOpNode(const string& op, ASTPtr left, ASTPtr right)
        : op(op), left(move(left)), right(move(right)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "BinaryOp(" << op << ")\n";
        left->print(indent + 2);
        right->print(indent + 2);
    }
};

struct DeclarationNode : ASTNode {
    string type;
    string name;
    ASTPtr initializer;

    DeclarationNode(const string& type, const string& name, ASTPtr initializer)
        : type(type), name(name), initializer(move(initializer)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ')
             << "Declaration(" << type << ", " << name << ")\n";

        if (initializer) {
            initializer->print(indent + 2);
        }
    }
};

struct AssignmentNode : ASTNode {
    string name;
    ASTPtr expression;

    AssignmentNode(const string& name, ASTPtr expression)
        : name(name), expression(move(expression)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ')
             << "Assignment(" << name << ")\n";
        expression->print(indent + 2);
    }
};

struct PrintNode : ASTNode {
    ASTPtr expression;

    explicit PrintNode(ASTPtr expression)
        : expression(move(expression)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "Print\n";
        expression->print(indent + 2);
    }
};

struct IfNode : ASTNode {
    ASTPtr condition;
    vector<ASTPtr> thenBranch;
    vector<ASTPtr> elseBranch;

    IfNode(ASTPtr condition,
           vector<ASTPtr> thenBranch,
           vector<ASTPtr> elseBranch)
        : condition(move(condition)),
          thenBranch(move(thenBranch)),
          elseBranch(move(elseBranch)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "If\n";

        cout << string(indent + 2, ' ') << "Condition:\n";
        condition->print(indent + 4);

        cout << string(indent + 2, ' ') << "Then:\n";
        for (const auto& stmt : thenBranch) {
            stmt->print(indent + 4);
        }

        if (!elseBranch.empty()) {
            cout << string(indent + 2, ' ') << "Else:\n";
            for (const auto& stmt : elseBranch) {
                stmt->print(indent + 4);
            }
        }
    }
};

struct WhileNode : ASTNode {
    ASTPtr condition;
    vector<ASTPtr> body;

    WhileNode(ASTPtr condition, vector<ASTPtr> body)
        : condition(move(condition)),
          body(move(body)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "While\n";

        cout << string(indent + 2, ' ') << "Condition:\n";
        condition->print(indent + 4);

        cout << string(indent + 2, ' ') << "Body:\n";
        for (const auto& stmt : body) {
            stmt->print(indent + 4);
        }
    }
};

struct ProgramNode : ASTNode {
    vector<ASTPtr> statements;

    explicit ProgramNode(vector<ASTPtr> statements)
        : statements(move(statements)) {}

    void print(int indent = 0) const override {
        cout << string(indent, ' ') << "Program\n";
        for (const auto& stmt : statements) {
            stmt->print(indent + 2);
        }
    }
};

// ============================================================
// PARSER
//
// Grammar:
//
// program       -> "শুরু" statement* "শেষ" EOF
//
// statement     -> declaration
//                | assignment
//                | print
//                | if_statement
//                | while_statement
//
// declaration   -> ("সংখ্যা" | "দশমিক") IDENTIFIER
//                  ["=" expression] ";"
//
// assignment    -> IDENTIFIER "=" expression ";"
//
// print         -> "দেখাও" "(" expression ")" ";"
//
// if_statement  -> "যদি" "(" expression ")" block
//                  ["নাহলে" block]
//
// while_statement -> "যতক্ষণ" "(" expression ")" block
//
// block         -> "{" statement* "}"
//
// expression    -> comparison
// comparison    -> addition ((">" | "<" | ">=" | "<=" |
//                              "==" | "!=") addition)*
//
// addition      -> multiplication (("+" | "-") multiplication)*
// multiplication -> unary (("*" | "/" | "%") unary)*
//
// unary         -> ("+" | "-") unary | primary
//
// primary       -> NUMBER | IDENTIFIER | "(" expression ")"
// ============================================================

class Parser {
private:
    vector<Token> tokens;
    size_t current;

    const Token& peek() const {
        return tokens[current];
    }

    bool isAtEnd() const {
        return peek().type == END_OF_FILE;
    }

    const Token& advance() {
        if (!isAtEnd()) {
            current++;
        }
        return tokens[current - 1];
    }

    bool check(TokenType type) const {
        if (isAtEnd()) {
            return type == END_OF_FILE;
        }
        return peek().type == type;
    }

    bool checkKeyword(const string& keyword) const {
        return peek().type == KEYWORD && peek().value == keyword;
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool matchKeyword(const string& keyword) {
        if (checkKeyword(keyword)) {
            advance();
            return true;
        }
        return false;
    }

    void error(const string& message) const {
        throw runtime_error(
            "Parser error at line " +
            to_string(peek().line) + ": " + message
        );
    }

    Token consume(TokenType type, const string& message) {
        if (check(type)) {
            return advance();
        }

        error(message);
        return Token{};
    }

    void consumeKeyword(const string& keyword, const string& message) {
        if (!matchKeyword(keyword)) {
            error(message);
        }
    }

    void synchronize() {
        // Basic error recovery:
        // skip tokens until semicolon, closing brace, or end of file.
        while (!isAtEnd()) {
            if (match(SEMICOLON)) {
                return;
            }

            if (check(RIGHT_BRACE)) {
                return;
            }

            advance();
        }
    }

    // --------------------------------------------------------
    // Statements
    // --------------------------------------------------------

    ASTPtr declaration() {
        string type;

        if (matchKeyword("সংখ্যা")) {
            type = "সংখ্যা";
        } else if (matchKeyword("দশমিক")) {
            type = "দশমিক";
        } else {
            error("Expected data type.");
        }

        Token name = consume(
            IDENTIFIER,
            "Expected identifier after data type."
        );

        ASTPtr initializer = nullptr;

        if (match(ASSIGN)) {
            initializer = expression();
        }

        consume(
            SEMICOLON,
            "Expected ';' after declaration."
        );

        return make_shared<DeclarationNode>(
            type,
            name.value,
            initializer
        );
    }

    ASTPtr assignment() {
        Token name = consume(
            IDENTIFIER,
            "Expected identifier."
        );

        consume(
            ASSIGN,
            "Expected '=' after identifier."
        );

        ASTPtr value = expression();

        consume(
            SEMICOLON,
            "Expected ';' after assignment."
        );

        return make_shared<AssignmentNode>(
            name.value,
            value
        );
    }

    ASTPtr printStatement() {
        consumeKeyword(
            "দেখাও",
            "Expected 'দেখাও'."
        );

        consume(
            LEFT_PAREN,
            "Expected '(' after 'দেখাও'."
        );

        ASTPtr value = expression();

        consume(
            RIGHT_PAREN,
            "Expected ')' after expression."
        );

        consume(
            SEMICOLON,
            "Expected ';' after print statement."
        );

        return make_shared<PrintNode>(value);
    }

    ASTPtr ifStatement() {
        consumeKeyword(
            "যদি",
            "Expected 'যদি'."
        );

        consume(
            LEFT_PAREN,
            "Expected '(' after 'যদি'."
        );

        ASTPtr condition = expression();

        consume(
            RIGHT_PAREN,
            "Expected ')' after condition."
        );

        vector<ASTPtr> thenBranch = block();

        vector<ASTPtr> elseBranch;

        if (matchKeyword("নাহলে")) {
            elseBranch = block();
        }

        return make_shared<IfNode>(
            condition,
            move(thenBranch),
            move(elseBranch)
        );
    }

    ASTPtr whileStatement() {
        consumeKeyword(
            "যতক্ষণ",
            "Expected 'যতক্ষণ'."
        );

        consume(
            LEFT_PAREN,
            "Expected '(' after 'যতক্ষণ'."
        );

        ASTPtr condition = expression();

        consume(
            RIGHT_PAREN,
            "Expected ')' after condition."
        );

        vector<ASTPtr> body = block();

        return make_shared<WhileNode>(
            condition,
            move(body)
        );
    }

    vector<ASTPtr> block() {
        consume(
            LEFT_BRACE,
            "Expected '{' before block."
        );

        vector<ASTPtr> statements;

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            try {
                statements.push_back(statement());
            } catch (const runtime_error& e) {
                cerr << e.what() << '\n';
                synchronize();
            }
        }

        consume(
            RIGHT_BRACE,
            "Expected '}' after block."
        );

        return statements;
    }

    ASTPtr statement() {
        if (checkKeyword("সংখ্যা") ||
            checkKeyword("দশমিক")) {
            return declaration();
        }

        if (checkKeyword("দেখাও")) {
            return printStatement();
        }

        if (checkKeyword("যদি")) {
            return ifStatement();
        }

        if (checkKeyword("যতক্ষণ")) {
            return whileStatement();
        }

        if (check(IDENTIFIER)) {
            return assignment();
        }

        error("Unexpected token '" + peek().value + "'.");
        return nullptr;
    }

    // --------------------------------------------------------
    // Expressions
    // --------------------------------------------------------

    ASTPtr expression() {
        return comparison();
    }

    ASTPtr comparison() {
        ASTPtr left = addition();

        while (check(GREATER) ||
               check(LESS) ||
               check(GREATER_EQUAL) ||
               check(LESS_EQUAL) ||
               check(EQUAL) ||
               check(NOT_EQUAL)) {

            Token op = advance();
            ASTPtr right = addition();

            left = make_shared<BinaryOpNode>(
                op.value,
                left,
                right
            );
        }

        return left;
    }

    ASTPtr addition() {
        ASTPtr left = multiplication();

        while (check(PLUS) || check(MINUS)) {
            Token op = advance();
            ASTPtr right = multiplication();

            left = make_shared<BinaryOpNode>(
                op.value,
                left,
                right
            );
        }

        return left;
    }

    ASTPtr multiplication() {
        ASTPtr left = unary();

        while (check(MULTIPLY) ||
               check(DIVIDE) ||
               check(MOD)) {

            Token op = advance();
            ASTPtr right = unary();

            left = make_shared<BinaryOpNode>(
                op.value,
                left,
                right
            );
        }

        return left;
    }

    ASTPtr unary() {
        if (check(PLUS) || check(MINUS)) {
            Token op = advance();

            ASTPtr zero = make_shared<NumberNode>("0");
            ASTPtr right = unary();

            return make_shared<BinaryOpNode>(
                op.value,
                zero,
                right
            );
        }

        return primary();
    }

    ASTPtr primary() {
        if (match(NUMBER)) {
            return make_shared<NumberNode>(
                tokens[current - 1].value
            );
        }

        if (match(IDENTIFIER)) {
            return make_shared<IdentifierNode>(
                tokens[current - 1].value
            );
        }

        if (match(LEFT_PAREN)) {
            ASTPtr expr = expression();

            consume(
                RIGHT_PAREN,
                "Expected ')' after expression."
            );

            return expr;
        }

        error("Expected expression.");
        return nullptr;
    }

public:
    explicit Parser(const vector<Token>& tokenList)
        : tokens(tokenList), current(0) {}

    shared_ptr<ProgramNode> parse() {
        // Optional program start keyword.
        if (checkKeyword("শুরু")) {
            advance();
        }

        vector<ASTPtr> statements;

        while (!isAtEnd() && !checkKeyword("শেষ")) {
            try {
                statements.push_back(statement());
            } catch (const runtime_error& e) {
                cerr << e.what() << '\n';
                synchronize();
            }
        }

        // Optional program end keyword.
        if (checkKeyword("শেষ")) {
            advance();
        }

        if (!isAtEnd()) {
            error("Unexpected tokens after program end.");
        }

        return make_shared<ProgramNode>(
            move(statements)
        );
    }
};

// ============================================================
// TEST
// ============================================================

int main() {
    string source =
        "শুরু\n"
        "সংখ্যা x = 10;\n"
        "দশমিক y = 5.5;\n"
        "যদি (x > 5) {\n"
        "    দেখাও(x);\n"
        "} নাহলে {\n"
        "    দেখাও(y);\n"
        "}\n"
        "যতক্ষণ (x < 20) {\n"
        "    x = x + 1;\n"
        "}\n"
        "শেষ";

    Lexer lexer(source);
    vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    shared_ptr<ProgramNode> ast = parser.parse();

    cout << "===== PARSE TREE / AST =====\n";
    ast->print();

    return 0;
}
