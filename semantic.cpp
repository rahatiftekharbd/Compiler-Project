#define LEXER_NO_MAIN
#define PARSER_NO_MAIN
#include "parser.cpp"

#include <cmath>
#include <sstream>
#include <unordered_map>

using namespace std;

enum class SemanticType {
    INTEGER,
    DECIMAL,
    BOOLEAN,
    INVALID
};

string semanticTypeName(SemanticType type) {
    switch (type) {
        case SemanticType::INTEGER: return "সংখ্যা";
        case SemanticType::DECIMAL: return "দশমিক";
        case SemanticType::BOOLEAN: return "condition";
        case SemanticType::INVALID: return "invalid";
    }

    return "invalid";
}

class SemanticAnalyzer {
private:
    vector<unordered_map<string, SemanticType>> scopes;
    vector<string> errors;

    void report(const string& message) {
        errors.push_back("Semantic error: " + message);
    }

    void beginScope() {
        scopes.push_back({});
    }

    void endScope() {
        scopes.pop_back();
    }

    SemanticType lookup(const string& name) const {
        for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
            auto found = scope->find(name);
            if (found != scope->end()) {
                return found->second;
            }
        }

        return SemanticType::INVALID;
    }

    bool isNumeric(SemanticType type) const {
        return type == SemanticType::INTEGER ||
               type == SemanticType::DECIMAL;
    }

    bool canAssign(SemanticType target, SemanticType value) const {
        if (target == SemanticType::DECIMAL &&
            value == SemanticType::INTEGER) {
            return true;
        }

        return target == value;
        
    }

    SemanticType checkExpression(const ASTPtr& expression) {
        if (auto number = dynamic_pointer_cast<NumberNode>(expression)) {
            return number->value.find('.') == string::npos
                ? SemanticType::INTEGER
                : SemanticType::DECIMAL;
        }

        if (auto identifier = dynamic_pointer_cast<IdentifierNode>(expression)) {
            SemanticType type = lookup(identifier->name);
            if (type == SemanticType::INVALID) {
                report("'" + identifier->name + "' is not declared.");
            }
            return type;
        }

        auto binary = dynamic_pointer_cast<BinaryOpNode>(expression);
        if (!binary) {
            report("Unknown expression.");
            return SemanticType::INVALID;
        }

        SemanticType left = checkExpression(binary->left);
        SemanticType right = checkExpression(binary->right);

        if (binary->op == ">" || binary->op == "<" ||
            binary->op == ">=" || binary->op == "<=" ||
            binary->op == "==" || binary->op == "!=") {
            if (!isNumeric(left) || !isNumeric(right)) {
                report("Comparison '" + binary->op +
                       "' requires two numeric expressions.");
            }
            return SemanticType::BOOLEAN;
        }

        if (!isNumeric(left) || !isNumeric(right)) {
            report("Operator '" + binary->op +
                   "' requires numeric expressions.");
            return SemanticType::INVALID;
        }

        if (binary->op == "%" &&
            (left != SemanticType::INTEGER || right != SemanticType::INTEGER)) {
            report("Operator '%' requires integer expressions.");
        }

        if (binary->op == "/") {
            auto divisor = dynamic_pointer_cast<NumberNode>(binary->right);
            if (divisor && stod(divisor->value) == 0.0) {
                report("Division by zero is not allowed.");
            }
        }

        return left == SemanticType::DECIMAL || right == SemanticType::DECIMAL
            ? SemanticType::DECIMAL
            : SemanticType::INTEGER;
    }

    void checkStatement(const ASTPtr& statement) {
        if (auto declaration = dynamic_pointer_cast<DeclarationNode>(statement)) {
            SemanticType type = declaration->type == "সংখ্যা"
                ? SemanticType::INTEGER
                : SemanticType::DECIMAL;

            if (scopes.back().find(declaration->name) != scopes.back().end()) {
                report("'" + declaration->name +
                       "' is already declared in this scope.");
            } else {
                scopes.back()[declaration->name] = type;
            }

            if (declaration->initializer) {
                SemanticType value = checkExpression(declaration->initializer);
                if (value != SemanticType::INVALID &&
                    !canAssign(type, value)) {
                    report("cannot initialize " + semanticTypeName(type) +
                           " variable '" + declaration->name + "' with " +
                           semanticTypeName(value) + " value.");
                }
            }
            return;
        }

        if (auto assignment = dynamic_pointer_cast<AssignmentNode>(statement)) {
            SemanticType target = lookup(assignment->name);
            if (target == SemanticType::INVALID) {
                report("'" + assignment->name + "' is not declared.");
            }

            SemanticType value = checkExpression(assignment->expression);
            if (target != SemanticType::INVALID &&
                value != SemanticType::INVALID &&
                !canAssign(target, value)) {
                report("cannot assign " + semanticTypeName(value) +
                       " value to " + semanticTypeName(target) +
                       " variable '" + assignment->name + "'.");
            }
            return;
        }

        if (auto print = dynamic_pointer_cast<PrintNode>(statement)) {
            checkExpression(print->expression);
            return;
        }

        if (auto ifNode = dynamic_pointer_cast<IfNode>(statement)) {
            if (checkExpression(ifNode->condition) != SemanticType::BOOLEAN) {
                report("'যদি' condition must be a comparison.");
            }
            checkBlock(ifNode->thenBranch);
            checkBlock(ifNode->elseBranch);
            return;
        }

        if (auto whileNode = dynamic_pointer_cast<WhileNode>(statement)) {
            if (checkExpression(whileNode->condition) != SemanticType::BOOLEAN) {
                report("'যতক্ষণ' condition must be a comparison.");
            }
            checkBlock(whileNode->body);
        }
    }

    void checkBlock(const vector<ASTPtr>& statements) {
        beginScope();
        for (const ASTPtr& statement : statements) {
            checkStatement(statement);
        }
        endScope();
    }

public:
    const vector<string>& analyze(const shared_ptr<ProgramNode>& program) {
        errors.clear();
        scopes.clear();
        beginScope();

        for (const ASTPtr& statement : program->statements) {
            checkStatement(statement);
        }

        endScope();
        return errors;
    }
};

int main() {
    const string source =
        "শুরু\n"
        "সংখ্যা x = 10;\n"
        "দশমিক y = x + 5.5;\n"
        "যদি (x > 5) {\n"
        "    দেখাও(y);\n"
        "}\n"
        "শেষ";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    shared_ptr<ProgramNode> program = parser.parse();

    SemanticAnalyzer analyzer;
    const vector<string>& errors = analyzer.analyze(program);

    if (errors.empty()) {
        cout << "Semantic analysis successful.\n";
        return 0;
    }

    for (const string& error : errors) {
        cerr << error << '\n';
    }

    return 1;
}
