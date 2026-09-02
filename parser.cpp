```cpp
#include <iostream>
#include <string>
#include <vector>
#include "lexer.cpp"

using namespace std;

class Parser {
private:
    vector<Token> tokens;
    int position;

    Token currentToken() {
        return tokens[position];
    }

    void advance() {
        if (position < tokens.size() - 1)
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

    void parse() {
        if (!matchKeyword(L"শুরু")) {
            error(L"Program must start with শুরু");
            return;
        }

        while (!checkKeyword(L"শেষ") &&
               !check(END_OF_FILE)) {
            statement();
        }

        if (!matchKeyword(L"শেষ")) {
            error(L"Program must end with শেষ");
            return;
        }

        cout << "Parsing completed successfully." << endl;
    }

    void statement() {
        if (checkKeyword(L"সংখ্যা") ||
            checkKeyword(L"দশমিক")) {
            declaration();
        }
        else if (checkKeyword(L"যদি")) {
            ifStatement();
        }
        else if (checkKeyword(L"যতক্ষণ")) {
            whileStatement();
        }
        else if (checkKeyword(L"দেখাও")) {
            printStatement();
        }
        else if (check(IDENTIFIER)) {
            assignment();
        }
        else {
            error(L"Invalid statement");
            skipLine();
        }
    }

    void declaration() {
        advance();

        if (!check(IDENTIFIER)) {
            error(L"Expected variable name");
            skipLine();
            return;
        }

        advance();

        if (!match(ASSIGN)) {
            error(L"Expected =");
            skipLine();
            return;
        }

        expression();

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
        }
    }

    void assignment() {
        advance();

        if (!match(ASSIGN)) {
            error(L"Expected =");
            skipLine();
            return;
        }

        expression();

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
        }
    }

    void ifStatement() {
        matchKeyword(L"যদি");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return;
        }

        condition();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            return;
        }

        if (!match(LEFT_BRACE)) {
            error(L"Expected {");
            skipLine();
            return;
        }

        while (!check(RIGHT_BRACE) &&
               !check(END_OF_FILE)) {
            statement();
        }

        if (!match(RIGHT_BRACE)) {
            error(L"Expected }");
            return;
        }

        if (checkKeyword(L"নাহলে")) {
            matchKeyword(L"নাহলে");

            if (!match(LEFT_BRACE)) {
                error(L"Expected {");
                skipLine();
                return;
            }

            while (!check(RIGHT_BRACE) &&
                   !check(END_OF_FILE)) {
                statement();
            }

            if (!match(RIGHT_BRACE)) {
                error(L"Expected }");
            }
        }
    }

    void whileStatement() {
        matchKeyword(L"যতক্ষণ");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return;
        }

        condition();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            return;
        }

        if (!match(LEFT_BRACE)) {
            error(L"Expected {");
            skipLine();
            return;
        }

        while (!check(RIGHT_BRACE) &&
               !check(END_OF_FILE)) {
            statement();
        }

        if (!match(RIGHT_BRACE)) {
            error(L"Expected }");
        }
    }

    void printStatement() {
        matchKeyword(L"দেখাও");

        if (!match(LEFT_PAREN)) {
            error(L"Expected (");
            skipLine();
            return;
        }

        expression();

        if (!match(RIGHT_PAREN)) {
            error(L"Expected )");
            skipLine();
            return;
        }

        if (!match(SEMICOLON)) {
            error(L"Expected ;");
            skipLine();
        }
    }

    void condition() {
        expression();

        if (check(GREATER) ||
            check(LESS) ||
            check(GREATER_EQUAL) ||
            check(LESS_EQUAL) ||
            check(EQUAL) ||
            check(NOT_EQUAL)) {

            advance();
            expression();
        }
        else {
            error(L"Expected comparison operator");
        }
    }

    void expression() {
        term();

        while (check(PLUS) || check(MINUS)) {
            advance();
            term();
        }
    }

    void term() {
        factor();

        while (check(MULTIPLY) ||
               check(DIVIDE) ||
               check(MOD)) {
            advance();
            factor();
        }
    }

    void factor() {
        if (check(NUMBER) ||
            check(IDENTIFIER)) {
            advance();
        }
        else if (match(LEFT_PAREN)) {
            expression();

            if (!match(RIGHT_PAREN)) {
                error(L"Expected )");
            }
        }
        else {
            error(L"Expected number or variable");
            advance();
        }
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

    vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens);

    parser.parse();

    return 0;
}
```