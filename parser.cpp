#include "lexer.cpp"

// ---------- AST nodes ----------
class ASTNode {
public:
    string type;   // data type of an expression (filled in by the semantic analyzer)
    virtual ~ASTNode() {}
};

class NumberNode : public ASTNode {
public:
    string value;
    NumberNode(string v) { value = v; }
};

class IdentifierNode : public ASTNode {
public:
    string name;
    IdentifierNode(string n) { name = n; }
};

class BinaryNode : public ASTNode {
public:
    string op;
    ASTNode* left;
    ASTNode* right;
    BinaryNode(string o, ASTNode* l, ASTNode* r) { op = o; left = l; right = r; }
};

class DeclarationNode : public ASTNode {
public:
    string varType, name;
    ASTNode* value;
    DeclarationNode(string t, string n, ASTNode* v) { varType = t; name = n; value = v; }
};

class AssignmentNode : public ASTNode {
public:
    string name;
    string varType;   // filled in by the semantic analyzer
    ASTNode* value;
    AssignmentNode(string n, ASTNode* v) { name = n; value = v; }
};

class PrintNode : public ASTNode {
public:
    ASTNode* expression;
    PrintNode(ASTNode* e) { expression = e; }
};

class IfNode : public ASTNode {
public:
    ASTNode* condition;
    vector<ASTNode*> ifBody, elseBody;
};

class WhileNode : public ASTNode {
public:
    ASTNode* condition;
    vector<ASTNode*> body;
};

class ProgramNode : public ASTNode {
public:
    vector<ASTNode*> statements;
};

// ---------- Parser ----------
int syntaxErrors = 0;

class Parser {
    vector<Token> tokens;
    int pos = 0;

    Token current() { return tokens[pos]; }
    void advance() { if (pos < (int)tokens.size() - 1) pos++; }
    bool check(TokenType t) { return current().type == t; }
    bool checkKeyword(string w) { return current().type == KEYWORD && current().value == w; }
    bool match(TokenType t) {
        if (check(t)) { advance(); return true; }
        return false;
    }

    void error(string message) {
        syntaxErrors++;
        cerr << "Syntax Error (line " << current().line << "): " << message << endl;
    }

    // Error recovery: skip tokens up to and including the next semicolon
    void skipToSemicolon() {
        while (!check(SEMICOLON) && !check(RIGHT_BRACE) && !check(END_OF_FILE)) advance();
        match(SEMICOLON);
    }

    bool expectSemicolon() {
        if (match(SEMICOLON)) return true;
        error("Expected ;");
        skipToSemicolon();
        return false;
    }

    // { statements }  -> fills body, returns false on error
    bool block(vector<ASTNode*>& body) {
        if (!match(LEFT_BRACE)) { error("Expected {"); skipToSemicolon(); return false; }
        while (!check(RIGHT_BRACE) && !check(END_OF_FILE) && !checkKeyword("শেষ")) {
            ASTNode* s = statement();
            if (s != nullptr) body.push_back(s);
        }
        if (!match(RIGHT_BRACE)) { error("Expected }"); return false; }
        return true;
    }

    ASTNode* statement() {
        if (checkKeyword("সংখ্যা") || checkKeyword("দশমিক")) return declaration();
        if (checkKeyword("যদি")) return ifStatement();
        if (checkKeyword("যতক্ষণ")) return whileStatement();
        if (checkKeyword("দেখাও")) return printStatement();
        if (check(IDENTIFIER)) return assignment();
        error("Invalid statement");
        advance();
        skipToSemicolon();
        return nullptr;
    }

    ASTNode* declaration() {
        string type = current().value;
        advance();
        if (!check(IDENTIFIER)) { error("Expected variable name"); skipToSemicolon(); return nullptr; }
        string name = current().value;
        advance();
        if (!match(ASSIGN)) { error("Expected ="); skipToSemicolon(); return nullptr; }
        ASTNode* value = expression();
        if (value == nullptr) { skipToSemicolon(); return nullptr; }
        if (!expectSemicolon()) return nullptr;
        return new DeclarationNode(type, name, value);
    }

    ASTNode* assignment() {
        string name = current().value;
        advance();
        if (!match(ASSIGN)) { error("Expected ="); skipToSemicolon(); return nullptr; }
        ASTNode* value = expression();
        if (value == nullptr) { skipToSemicolon(); return nullptr; }
        if (!expectSemicolon()) return nullptr;
        return new AssignmentNode(name, value);
    }

    ASTNode* printStatement() {
        advance();  // দেখাও
        if (!match(LEFT_PAREN)) { error("Expected ("); skipToSemicolon(); return nullptr; }
        ASTNode* value = expression();
        if (value == nullptr) { skipToSemicolon(); return nullptr; }
        if (!match(RIGHT_PAREN)) { error("Expected )"); skipToSemicolon(); return nullptr; }
        if (!expectSemicolon()) return nullptr;
        return new PrintNode(value);
    }

    // যদি ( condition ) { ... } নাহলে { ... }
    ASTNode* ifStatement() {
        advance();  // যদি
        if (!match(LEFT_PAREN)) { error("Expected ("); skipToSemicolon(); return nullptr; }
        ASTNode* cond = condition();
        if (cond == nullptr) { skipToSemicolon(); return nullptr; }
        if (!match(RIGHT_PAREN)) { error("Expected )"); skipToSemicolon(); return nullptr; }

        IfNode* node = new IfNode();
        node->condition = cond;
        if (!block(node->ifBody)) return nullptr;
        if (checkKeyword("নাহলে")) {
            advance();
            if (!block(node->elseBody)) return nullptr;
        }
        return node;
    }

    // যতক্ষণ ( condition ) { ... }
    ASTNode* whileStatement() {
        advance();  // যতক্ষণ
        if (!match(LEFT_PAREN)) { error("Expected ("); skipToSemicolon(); return nullptr; }
        ASTNode* cond = condition();
        if (cond == nullptr) { skipToSemicolon(); return nullptr; }
        if (!match(RIGHT_PAREN)) { error("Expected )"); skipToSemicolon(); return nullptr; }

        WhileNode* node = new WhileNode();
        node->condition = cond;
        if (!block(node->body)) return nullptr;
        return node;
    }

    // expression comparison-operator expression
    ASTNode* condition() {
        ASTNode* left = expression();
        if (left == nullptr) return nullptr;
        if (check(GREATER) || check(LESS) || check(GREATER_EQUAL) ||
            check(LESS_EQUAL) || check(EQUAL) || check(NOT_EQUAL)) {
            string op = current().value;
            advance();
            ASTNode* right = expression();
            if (right == nullptr) return nullptr;
            return new BinaryNode(op, left, right);
        }
        error("Expected a comparison operator (>, <, >=, <=, ==, !=)");
        return nullptr;
    }

    // + and - (lowest precedence)
    ASTNode* expression() {
        ASTNode* left = term();
        while (left != nullptr && (check(PLUS) || check(MINUS))) {
            string op = current().value;
            advance();
            ASTNode* right = term();
            if (right == nullptr) return nullptr;
            left = new BinaryNode(op, left, right);
        }
        return left;
    }

    // * / % (higher precedence)
    ASTNode* term() {
        ASTNode* left = factor();
        while (left != nullptr && (check(MULTIPLY) || check(DIVIDE) || check(MOD))) {
            string op = current().value;
            advance();
            ASTNode* right = factor();
            if (right == nullptr) return nullptr;
            left = new BinaryNode(op, left, right);
        }
        return left;
    }

    // number, variable, -factor, or ( expression )
    ASTNode* factor() {
        if (check(NUMBER)) {
            string value = current().value;
            advance();
            return new NumberNode(value);
        }
        if (check(IDENTIFIER)) {
            string name = current().value;
            advance();
            return new IdentifierNode(name);
        }
        if (match(MINUS)) {
            ASTNode* inner = factor();
            if (inner == nullptr) return nullptr;
            return new BinaryNode("-", new NumberNode("0"), inner);
        }
        if (match(LEFT_PAREN)) {
            ASTNode* inner = expression();
            if (inner == nullptr) return nullptr;
            if (!match(RIGHT_PAREN)) { error("Expected )"); return nullptr; }
            return inner;
        }
        error("Expected a number or variable");
        return nullptr;
    }

public:
    Parser(vector<Token> input) { tokens = input; }

    // শুরু statements শেষ
    ProgramNode* parse() {
        if (!checkKeyword("শুরু")) { error("Program must start with শুরু"); return nullptr; }
        advance();
        ProgramNode* program = new ProgramNode();
        while (!checkKeyword("শেষ") && !check(END_OF_FILE)) {
            ASTNode* s = statement();
            if (s != nullptr) program->statements.push_back(s);
        }
        if (!checkKeyword("শেষ")) { error("Program must end with শেষ"); return program; }
        advance();
        if (!check(END_OF_FILE)) error("Unexpected code after শেষ");
        return program;
    }
};
