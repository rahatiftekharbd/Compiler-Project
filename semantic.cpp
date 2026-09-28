#include "parser.cpp"
#include <map>

const string INT_TYPE = "সংখ্যা";
const string DEC_TYPE = "দশমিক";
const string BOOL_TYPE = "শর্ত";   // result of a comparison
const string BAD_TYPE = "error";    // an error was already reported

class SemanticAnalyzer {
    map<string, string> variables;   // symbol table: variable name -> type
    int errors = 0;

    void report(string message) {
        errors++;
        cerr << "Semantic Error: " << message << endl;
    }

    bool isNumber(string t) { return t == INT_TYPE || t == DEC_TYPE; }

    // a decimal variable can hold an integer, but not the other way round
    bool canStore(string target, string value) {
        return target == value || (target == DEC_TYPE && value == INT_TYPE);
    }

    // returns the type of an expression (and saves it in the node)
    string checkExpression(ASTNode* node) {
        if (node == nullptr) return BAD_TYPE;

        NumberNode* number = dynamic_cast<NumberNode*>(node);
        if (number != nullptr) {
            node->type = (number->value.find('.') == string::npos) ? INT_TYPE : DEC_TYPE;
            return node->type;
        }

        IdentifierNode* id = dynamic_cast<IdentifierNode*>(node);
        if (id != nullptr) {
            if (variables.count(id->name) == 0) {
                report("variable '" + id->name + "' is not declared");
                node->type = BAD_TYPE;
            } else {
                node->type = variables[id->name];
            }
            return node->type;
        }

        BinaryNode* bin = dynamic_cast<BinaryNode*>(node);
        if (bin == nullptr) return BAD_TYPE;

        string left = checkExpression(bin->left);
        string right = checkExpression(bin->right);
        node->type = BAD_TYPE;
        if (left == BAD_TYPE || right == BAD_TYPE) return BAD_TYPE;

        if (!isNumber(left) || !isNumber(right)) {
            report("operator '" + bin->op + "' needs numbers on both sides");
            return BAD_TYPE;
        }

        bool isComparison = bin->op == ">" || bin->op == "<" || bin->op == ">=" ||
                            bin->op == "<=" || bin->op == "==" || bin->op == "!=";
        if (isComparison) {
            node->type = BOOL_TYPE;
            return node->type;
        }

        if (bin->op == "/" || bin->op == "%") {
            NumberNode* divisor = dynamic_cast<NumberNode*>(bin->right);
            if (divisor != nullptr && stod(divisor->value) == 0.0) {
                report("division by zero");
                return BAD_TYPE;
            }
        }

        node->type = (left == DEC_TYPE || right == DEC_TYPE) ? DEC_TYPE : INT_TYPE;
        return node->type;
    }

    void checkCondition(ASTNode* condition) {
        string t = checkExpression(condition);
        if (t != BOOL_TYPE && t != BAD_TYPE) report("condition must be a comparison");
    }

    void checkBlock(vector<ASTNode*>& statements) {
        for (ASTNode* s : statements) checkStatement(s);
    }

    void checkStatement(ASTNode* node) {
        if (node == nullptr) return;

        DeclarationNode* decl = dynamic_cast<DeclarationNode*>(node);
        if (decl != nullptr) {
            string valueType = checkExpression(decl->value);
            if (variables.count(decl->name) > 0)
                report("variable '" + decl->name + "' is already declared");
            else
                variables[decl->name] = decl->varType;
            if (valueType != BAD_TYPE && !canStore(decl->varType, valueType))
                report("cannot store " + valueType + " value in " + decl->varType + " variable '" + decl->name + "'");
            return;
        }

        AssignmentNode* assign = dynamic_cast<AssignmentNode*>(node);
        if (assign != nullptr) {
            string valueType = checkExpression(assign->value);
            if (variables.count(assign->name) == 0) {
                report("variable '" + assign->name + "' is not declared");
                return;
            }
            assign->varType = variables[assign->name];
            if (valueType != BAD_TYPE && !canStore(assign->varType, valueType))
                report("cannot store " + valueType + " value in " + assign->varType + " variable '" + assign->name + "'");
            return;
        }

        PrintNode* print = dynamic_cast<PrintNode*>(node);
        if (print != nullptr) {
            checkExpression(print->expression);
            return;
        }

        IfNode* ifNode = dynamic_cast<IfNode*>(node);
        if (ifNode != nullptr) {
            checkCondition(ifNode->condition);
            checkBlock(ifNode->ifBody);
            checkBlock(ifNode->elseBody);
            return;
        }

        WhileNode* whileNode = dynamic_cast<WhileNode*>(node);
        if (whileNode != nullptr) {
            checkCondition(whileNode->condition);
            checkBlock(whileNode->body);
        }
    }

public:
    // returns the number of errors found
    int analyze(ProgramNode* program) {
        checkBlock(program->statements);
        return errors;
    }
};
