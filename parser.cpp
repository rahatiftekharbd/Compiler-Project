#include <iostream>
#include <string>
#include <vector>
#include "lexer.cpp"

using namespace std;

class ASTNode {
public:
    virtual ~ASTNode() {}
};

class NumberNode : public ASTNode {
public:
    string value;

    NumberNode(string v) {
        value = v;
    }
};

class IdentifierNode : public ASTNode {
public:
    string name;

    IdentifierNode(string n) {
        name = n;
    }
};

class BinaryNode : public ASTNode {
public:
    string op;
    ASTNode* left;
    ASTNode* right;

    BinaryNode(string o, ASTNode* l, ASTNode* r) {
        op = o;
        left = l;
        right = r;
    }

    ~BinaryNode() {
        delete left;
        delete right;
    }
};

class DeclarationNode : public ASTNode {
public:
    string type;
    string name;
    ASTNode* value;

    DeclarationNode(string t, string n, ASTNode* v) {
        type = t;
        name = n;
        value = v;
    }

    ~DeclarationNode() {
        delete value;
    }
};

class AssignmentNode : public ASTNode {
public:
    string name;
    ASTNode* value;

    AssignmentNode(string n, ASTNode* v) {
        name = n;
        value = v;
    }

    ~AssignmentNode() {
        delete value;
    }
};

class PrintNode : public ASTNode {
public:
    ASTNode* expression;

    PrintNode(ASTNode* e) {
        expression = e;
    }

    ~PrintNode() {
        delete expression;
    }
};

class IfNode : public ASTNode {
public:
    ASTNode* condition;
    vector<ASTNode*> ifStatements;
    vector<ASTNode*> elseStatements;

    IfNode(
        ASTNode* c,
        vector<ASTNode*> ifBody,
        vector<ASTNode*> elseBody
    ) {
        condition = c;
        ifStatements = ifBody;
        elseStatements = elseBody;
    }

    ~IfNode() {
        delete condition;

        for (ASTNode* node : ifStatements)
            delete node;

        for (ASTNode* node : elseStatements)
            delete node;
    }
};

class WhileNode : public ASTNode {
public:
    ASTNode* condition;
    vector<ASTNode*> statements;

    WhileNode(
        ASTNode* c,
        vector<ASTNode*> body
    ) {
        condition = c;
        statements = body;
    }

    ~WhileNode() {
        delete condition;

        for (ASTNode* node : statements)
            delete node;
    }
};

class ProgramNode : public ASTNode {
public:
    vector<ASTNode*> statements;

    ~ProgramNode() {
        for (ASTNode* node : statements)
            delete node;
    }
};

class Parser {
private:
    vector<Token> tokens;
    int position;

    Token currentToken() {
        return tokens[position];
    }

    void advance() {
        if (position < (int)tokens.size() - 1)
            position++;
    }

    bool check(TokenType type) {
        return currentToken().type == type;
    }

    bool checkKeyword(wstring word) {
        return currentToken().type == KEYWORD &&
               currentToken().value == word;
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }

        return false;
    }

    bool matchKeyword(wstring word) {
        if (checkKeyword(word)) {
            advance();
            return true;
        }

        return false;
    }

    void error(wstring message) {
        wcerr << L"Syntax Error at Line "
              << currentToken().line
              << L": "
              << message
              << endl;
    }

public:
    Parser(vector<Token> input) {
        tokens = input;
        position = 0;
    }

    ProgramNode* parse() {
        ProgramNode* program = new ProgramNode();

        if (!matchKeyword(L"শুরু")) {
            error(L"Program must start with শুরু");
            delete program;
            return nullptr;
        }

        while (!checkKeyword(L"শেষ") &&
               !check(END_OF_FILE)) {

            ASTNode* node = statement();

            if (node != nullptr)
                program->statements.push_back(node);
        }

        if (!matchKeyword(L"শেষ")) {
            error(L"Program must end with শেষ");
            delete program;
            return nullptr;
        }

        cout << "Parsing completed successfully."
             << endl;

        return program;
    }

    ASTNode* statement() {
        if (checkKeyword(L"সংখ্যা") ||
            checkKeyword(L"দশমিক")) {

            return declaration();
        }

        else if (checkKeyword(L"যদি")) {
            return ifStatement();
        }

        else if (checkKeyword(L"যতক্ষণ")) {
            return whileStatement();
        }

        else if (checkKeyword(L"দেখাও")) {
            return printStatement();
        }

        else if (check(IDENTIFIER)) {
            return assignment();
        }

        else {
            error(L"Invalid statement");
            skipLine();
            return nullptr;
        }
    }

    ASTNode* declaration() {
        string type =
            string(
                currentToken().value.begin(),
                currentToken().value.end()
            );

        advance();

        if (!check(IDENTIFIER)) {
            error(L"Expected variable name");
            skipLine();
            return nullptr;
        }

        string name = currentToken().value;

        advance();

        if (!match(ASSIGN)) {
            error(L"Expected =");
            skipLine();
            return nullptr;
        }

        ASTNode* value = expression();

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
            delete value;
            return nullptr;
        }

        return new DeclarationNode(
            type,
            name,
            value
        );
    }

    ASTNode* assignment() {
        string name = currentToken().value;

        advance();

        if (!match(ASSIGN)) {
            error(L"Expected =");
            skipLine();
            return nullptr;
        }

        ASTNode* value = expression();

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
            delete value;
            return nullptr;
        }

        return new AssignmentNode(
            name,
            value
        );
    }

    ASTNode* ifStatement() {
        matchKeyword(L"যদি");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return nullptr;
        }

        ASTNode* conditionNode = condition();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            delete conditionNode;
            return nullptr;
        }

        if (!match(LEFT_BRACE)) {
            error(L"Expected {");
            skipLine();
            delete conditionNode;
            return nullptr;
        }

        vector<ASTNode*> ifBody;

        while (!check(RIGHT_BRACE) &&
               !check(END_OF_FILE)) {

            ASTNode* node = statement();

            if (node != nullptr)
                ifBody.push_back(node);
        }

        if (!match(RIGHT_BRACE)) {
            error(L"Expected }");

            delete conditionNode;

            for (ASTNode* node : ifBody)
                delete node;

            return nullptr;
        }

        vector<ASTNode*> elseBody;

        if (checkKeyword(L"নাহলে")) {
            matchKeyword(L"নাহলে");

            if (!match(LEFT_BRACE)) {
                error(L"Expected {");

                delete conditionNode;

                for (ASTNode* node : ifBody)
                    delete node;

                return nullptr;
            }

            while (!check(RIGHT_BRACE) &&
                   !check(END_OF_FILE)) {

                ASTNode* node = statement();

                if (node != nullptr)
                    elseBody.push_back(node);
            }

            if (!match(RIGHT_BRACE)) {
                error(L"Expected }");

                delete conditionNode;

                for (ASTNode* node : ifBody)
                    delete node;

                for (ASTNode* node : elseBody)
                    delete node;

                return nullptr;
            }
        }

        return new IfNode(
            conditionNode,
            ifBody,
            elseBody
        );
    }

    ASTNode* whileStatement() {
        matchKeyword(L"যতক্ষণ");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return nullptr;
        }

        ASTNode* conditionNode = condition();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            delete conditionNode;
            return nullptr;
        }

        if (!match(LEFT_BRACE)) {
            error(L"Expected {");
            skipLine();
            delete conditionNode;
            return nullptr;
        }

        vector<ASTNode*> body;

        while (!check(RIGHT_BRACE) &&
               !check(END_OF_FILE)) {

            ASTNode* node = statement();

            if (node != nullptr)
                body.push_back(node);
        }

        if (!match(RIGHT_BRACE)) {
            error(L"Expected }");

            delete conditionNode;

            for (ASTNode* node : body)
                delete node;

            return nullptr;
        }

        return new WhileNode(
            conditionNode,
            body
        );
    }

    ASTNode* printStatement() {
        matchKeyword(L"দেখাও");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return nullptr;
        }

        ASTNode* value = expression();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            delete value;
            return nullptr;
        }

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
            delete value;
            return nullptr;
        }

        return new PrintNode(value);
    }

    ASTNode* condition() {
        ASTNode* left = expression();

        if (check(GREATER) ||
            check(LESS) ||
            check(GREATER_EQUAL) ||
            check(LESS_EQUAL) ||
            check(EQUAL) ||
            check(NOT_EQUAL)) {

            string op = currentToken().value;

            advance();

            ASTNode* right = expression();

            return new BinaryNode(
                op,
                left,
                right
            );
        }

        error(L"Expected comparison operator");

        delete left;

        return nullptr;
    }

    ASTNode* expression() {
        ASTNode* left = term();

        while (check(PLUS) ||
               check(MINUS)) {

            string op = currentToken().value;

            advance();

            ASTNode* right = term();

            left = new BinaryNode(
                op,
                left,
                right
            );
        }

        return left;
    }

    ASTNode* term() {
        ASTNode* left = factor();

        while (check(MULTIPLY) ||
               check(DIVIDE) ||
               check(MOD)) {

            string op = currentToken().value;

            advance();

            ASTNode* right = factor();

            left = new BinaryNode(
                op,
                left,
                right
            );
        }

        return left;
    }

    ASTNode* factor() {
        if (check(NUMBER)) {

            string value = currentToken().value;

            advance();

            return new NumberNode(value);
        }

        if (check(IDENTIFIER)) {

            string name = currentToken().value;

            advance();

            return new IdentifierNode(name);
        }

        if (match(LEFT_PAREN)) {

            ASTNode* node = expression();

            if (!match(RIGHT_PAREN)) {
                error(L"Expected )");
                delete node;
                return nullptr;
            }

            return node;
        }

        error(L"Expected number or variable");

        advance();

        return nullptr;
    }

    void skipLine() {
        while (!check(SEMICOLON) &&
               !check(END_OF_FILE)) {

            advance();
        }

        if (check(SEMICOLON))
            advance();
    }
};

void printAST(ASTNode* node, int indent = 0) {
    if (node == nullptr)
        return;

    string spaces(indent, ' ');

    if (ProgramNode* program =
        dynamic_cast<ProgramNode*>(node)) {

        cout << spaces << "Program" << endl;

        for (ASTNode* child : program->statements)
            printAST(child, indent + 2);

        return;
    }

    if (DeclarationNode* declaration =
        dynamic_cast<DeclarationNode*>(node)) {

        cout << spaces
             << "Declaration: "
             << declaration->type
             << " "
             << declaration->name
             << endl;

        printAST(
            declaration->value,
            indent + 2
        );

        return;
    }

    if (AssignmentNode* assignment =
        dynamic_cast<AssignmentNode*>(node)) {

        cout << spaces
             << "Assignment: "
             << assignment->name
             << endl;

        printAST(
            assignment->value,
            indent + 2
        );

        return;
    }

    if (NumberNode* number =
        dynamic_cast<NumberNode*>(node)) {

        cout << spaces
             << "Number: "
             << number->value
             << endl;

        return;
    }

    if (IdentifierNode* identifier =
        dynamic_cast<IdentifierNode*>(node)) {

        cout << spaces
             << "Identifier: "
             << identifier->name
             << endl;

        return;
    }

    if (BinaryNode* binary =
        dynamic_cast<BinaryNode*>(node)) {

        cout << spaces
             << "Operator: "
             << binary->op
             << endl;

        printAST(
            binary->left,
            indent + 2
        );

        printAST(
            binary->right,
            indent + 2
        );

        return;
    }

    if (PrintNode* print =
        dynamic_cast<PrintNode*>(node)) {

        cout << spaces
             << "Print"
             << endl;

        printAST(
            print->expression,
            indent + 2
        );

        return;
    }

    if (IfNode* ifNode =
        dynamic_cast<IfNode*>(node)) {

        cout << spaces
             << "IF"
             << endl;

        cout << spaces
             << "Condition:"
             << endl;

        printAST(
            ifNode->condition,
            indent + 2
        );

        cout << spaces
             << "IF Body:"
             << endl;

        for (ASTNode* child :
             ifNode->ifStatements) {

            printAST(
                child,
                indent + 2
            );
        }

        if (!ifNode->elseStatements.empty()) {

            cout << spaces
                 << "ELSE Body:"
                 << endl;

            for (ASTNode* child :
                 ifNode->elseStatements) {

                printAST(
                    child,
                    indent + 2
                );
            }
        }

        return;
    }

    if (WhileNode* whileNode =
        dynamic_cast<WhileNode*>(node)) {

        cout << spaces
             << "WHILE"
             << endl;

        cout << spaces
             << "Condition:"
             << endl;

        printAST(
            whileNode->condition,
            indent + 2
        );

        cout << spaces
             << "Body:"
             << endl;

        for (ASTNode* child :
             whileNode->statements) {

            printAST(
                child,
                indent + 2
            );
        }

        return;
    }
}

int main() {

    locale::global(locale(""));
    wcout.imbue(locale(""));

    wstring source =
        L"শুরু\n"
        L"সংখ্যা x = 10;\n"
        L"দশমিক y = 5.5;\n"
        L"যদি (x > 5) {\n"
        L"    দেখাও(x);\n"
        L"} নাহলে {\n"
        L"    দেখাও(y);\n"
        L"}\n"
        L"যতক্ষণ (x < 20) {\n"
        L"    x = x + 1;\n"
        L"}\n"
        L"শেষ";

    Lexer lexer(source);

    vector<Token> tokens =
        lexer.tokenize();

    Parser parser(tokens);

    ProgramNode* program =
        parser.parse();

    if (program != nullptr) {

        cout << "\n===== AST =====\n";

        printAST(program);

        cout << "================\n";

        delete program;
    }

    return 0;
}
