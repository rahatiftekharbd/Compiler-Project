#include <iostream>
#include <string>
#include <vector>
#include <cctype>
using namespace std;

enum TokenType {
    KEYWORD, IDENTIFIER, NUMBER,
    PLUS, MINUS, MULTIPLY, DIVIDE, MOD, ASSIGN,
    GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, EQUAL, NOT_EQUAL,
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, SEMICOLON,
    END_OF_FILE, INVALID
};

struct Token {
    TokenType type;
    string value;
    int line;
};

const string KEYWORDS[] = {"শুরু", "শেষ", "সংখ্যা", "দশমিক", "যদি", "নাহলে", "যতক্ষণ", "দেখাও"};

class Lexer {
    string src;
    int pos = 0;
    int line = 1;

    // is the character after the current one equal to c?
    bool nextIs(char c) {
        return pos + 1 < (int)src.size() && src[pos + 1] == c;
    }

    Token readNumber() {
        string value;
        while (pos < (int)src.size() && isdigit((unsigned char)src[pos])) value += src[pos++];
        if (pos < (int)src.size() && src[pos] == '.') {
            value += src[pos++];
            while (pos < (int)src.size() && isdigit((unsigned char)src[pos])) value += src[pos++];
        }
        return {NUMBER, value, line};
    }

    Token readWord() {
        string value;
        // bytes >= 128 are parts of Bangla (UTF-8) letters
        while (pos < (int)src.size() &&
               (isalnum((unsigned char)src[pos]) || (unsigned char)src[pos] >= 128 || src[pos] == '_'))
            value += src[pos++];
        for (const string& k : KEYWORDS)
            if (value == k) return {KEYWORD, value, line};
        return {IDENTIFIER, value, line};
    }

    Token readSymbol() {
        char c = src[pos];
        bool hasEqual = nextIs('=');
        pos++;
        switch (c) {
            case '+': return {PLUS, "+", line};
            case '-': return {MINUS, "-", line};
            case '*': return {MULTIPLY, "*", line};
            case '/': return {DIVIDE, "/", line};
            case '%': return {MOD, "%", line};
            case '(': return {LEFT_PAREN, "(", line};
            case ')': return {RIGHT_PAREN, ")", line};
            case '{': return {LEFT_BRACE, "{", line};
            case '}': return {RIGHT_BRACE, "}", line};
            case ';': return {SEMICOLON, ";", line};
            case '=':
                if (hasEqual) { pos++; return {EQUAL, "==", line}; }
                return {ASSIGN, "=", line};
            case '>':
                if (hasEqual) { pos++; return {GREATER_EQUAL, ">=", line}; }
                return {GREATER, ">", line};
            case '<':
                if (hasEqual) { pos++; return {LESS_EQUAL, "<=", line}; }
                return {LESS, "<", line};
            case '!':
                if (hasEqual) { pos++; return {NOT_EQUAL, "!=", line}; }
                return {INVALID, "!", line};
        }
        return {INVALID, string(1, c), line};
    }

public:
    Lexer(string text) { src = text; }

    vector<Token> tokenize() {
        vector<Token> tokens;
        while (pos < (int)src.size()) {
            char c = src[pos];
            if (c == '\n') { line++; pos++; }
            else if (c == ' ' || c == '\t' || c == '\r') pos++;
            else if (isdigit((unsigned char)c)) tokens.push_back(readNumber());
            else if (isalpha((unsigned char)c) || (unsigned char)c >= 128 || c == '_') tokens.push_back(readWord());
            else tokens.push_back(readSymbol());
        }
        tokens.push_back({END_OF_FILE, "", line});
        return tokens;
    }
};

