
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>

using namespace std;

enum TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    ASSIGN,
    GREATER,
    LESS,
    GREATER_EQUAL,
    LESS_EQUAL,
    EQUAL,
    NOT_EQUAL,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    SEMICOLON,
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    string value;
    int line;
};

class Lexer {
private:
    string source;
    int position;
    int line;

    unordered_map<string, TokenType> keywords;

public:
    Lexer(string input) {
        source = input;
        position = 0;
        line = 1;

        keywords["শুরু"] = KEYWORD;
        keywords["শেষ"] = KEYWORD;
        keywords["সংখ্যা"] = KEYWORD;
        keywords["দশমিক"] = KEYWORD;
        keywords["যদি"] = KEYWORD;
        keywords["নাহলে"] = KEYWORD;
        keywords["যতক্ষণ"] = KEYWORD;
        keywords["দেখাও"] = KEYWORD;
    }

    bool isBanglaLetter(unsigned char c) {
        return c >= 128;
    }

    vector<Token> tokenize() {
        vector<Token> tokens;

        while (position < source.length()) {
            char current = source[position];

            if (current == ' ' || current == '\t' || current == '\r') {
                position++;
                continue;
            }

            if (current == '\n') {
                line++;
                position++;
                continue;
            }

            if (isdigit(current)) {
                tokens.push_back(readNumber());
                continue;
            }

            if (isalpha(current) || isBanglaLetter(current)) {
                tokens.push_back(readWord());
                continue;
            }

            if (current == '+') {
                tokens.push_back({PLUS, "+", line});
                position++;
            }
            else if (current == '-') {
                tokens.push_back({MINUS, "-", line});
                position++;
            }
            else if (current == '*') {
                tokens.push_back({MULTIPLY, "*", line});
                position++;
            }
            else if (current == '/') {
                tokens.push_back({DIVIDE, "/", line});
                position++;
            }
            else if (current == '%') {
                tokens.push_back({MOD, "%", line});
                position++;
            }
            else if (current == '=') {
                if (position + 1 < source.length() &&
                    source[position + 1] == '=') {
                    tokens.push_back({EQUAL, "==", line});
                    position += 2;
                }
                else {
                    tokens.push_back({ASSIGN, "=", line});
                    position++;
                }
            }
            else if (current == '>') {
                if (position + 1 < source.length() &&
                    source[position + 1] == '=') {
                    tokens.push_back({GREATER_EQUAL, ">=", line});
                    position += 2;
                }
                else {
                    tokens.push_back({GREATER, ">", line});
                    position++;
                }
            }
            else if (current == '<') {
                if (position + 1 < source.length() &&
                    source[position + 1] == '=') {
                    tokens.push_back({LESS_EQUAL, "<=", line});
                    position += 2;
                }
                else {
                    tokens.push_back({LESS, "<", line});
                    position++;
                }
            }
            else if (current == '!') {
                if (position + 1 < source.length() &&
                    source[position + 1] == '=') {
                    tokens.push_back({NOT_EQUAL, "!=", line});
                    position += 2;
                }
                else {
                    tokens.push_back({INVALID, "!", line});
                    position++;
                }
            }
            else if (current == '(') {
                tokens.push_back({LEFT_PAREN, "(", line});
                position++;
            }
            else if (current == ')') {
                tokens.push_back({RIGHT_PAREN, ")", line});
                position++;
            }
            else if (current == '{') {
                tokens.push_back({LEFT_BRACE, "{", line});
                position++;
            }
            else if (current == '}') {
                tokens.push_back({RIGHT_BRACE, "}", line});
                position++;
            }
            else if (current == ';') {
                tokens.push_back({SEMICOLON, ";", line});
                position++;
            }
            else {
                tokens.push_back({INVALID, string(1, current), line});
                position++;
            }
        }

        tokens.push_back({END_OF_FILE, "", line});

        return tokens;
    }

    Token readNumber() {
        string value;
        bool hasDecimal = false;

        while (position < source.length() &&
               isdigit(source[position])) {
            value += source[position];
            position++;
        }

        if (position < source.length() &&
            source[position] == '.') {

            hasDecimal = true;
            value += '.';
            position++;

            while (position < source.length() &&
                   isdigit(source[position])) {
                value += source[position];
                position++;
            }
        }

        return {NUMBER, value, line};
    }

    Token readWord() {
        string value;

        while (position < source.length()) {
            unsigned char current = source[position];

            if (isalnum(current) || current >= 128 || current == '_') {
                value += source[position];
                position++;
            }
            else {
                break;
            }
        }

        if (keywords.find(value) != keywords.end()) {
            return {KEYWORD, value, line};
        }

        return {IDENTIFIER, value, line};
    }
};

string tokenName(TokenType type) {
    switch (type) {
        case KEYWORD: return "KEYWORD";
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER: return "NUMBER";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MULTIPLY: return "MULTIPLY";
        case DIVIDE: return "DIVIDE";
        case MOD: return "MOD";
        case ASSIGN: return "ASSIGN";
        case GREATER: return "GREATER";
        case LESS: return "LESS";
        case GREATER_EQUAL: return "GREATER_EQUAL";
        case LESS_EQUAL: return "LESS_EQUAL";
        case EQUAL: return "EQUAL";
        case NOT_EQUAL: return "NOT_EQUAL";
        case LEFT_PAREN: return "LEFT_PAREN";
        case RIGHT_PAREN: return "RIGHT_PAREN";
        case LEFT_BRACE: return "LEFT_BRACE";
        case RIGHT_BRACE: return "RIGHT_BRACE";
        case SEMICOLON: return "SEMICOLON";
        case END_OF_FILE: return "EOF";
        case INVALID: return "INVALID";
    }

    return "UNKNOWN";
}

int main() {
    string source =
        "শুরু\n"
        "সংখ্যা x = 10;\n"
        "দশমিক y = 5.5;\n"
        "যদি (x > 5) {\n"
        "    দেখাও(x);\n"
        "}\n"
        "যতক্ষণ (x < 20) {\n"
        "    x = x + 1;\n"
        "}\n"
        "শেষ";

    Lexer lexer(source);

    vector<Token> tokens = lexer.tokenize();

    for (Token token : tokens) {
        cout << tokenName(token.type)
             << " : "
             << token.value
             << " : Line "
             << token.line
             << endl;
    }

    return 0;
}

