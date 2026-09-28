// Build:  g++ -std=c++17 main.cpp -o bangla
// Run:    ./bangla program.bangla output.py
#include "codegen.cpp"
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    string inputFile = (argc > 1) ? argv[1] : "program.bangla";
    string outputFile = (argc > 2) ? argv[2] : "output.py";

    try {
        ifstream in(inputFile);
        if (!in) {
            cerr << "Cannot open file: " << inputFile << endl;
            return 1;
        }
        stringstream buffer;
        buffer << in.rdbuf();
        string source = buffer.str();

        // remove the invisible UTF-8 BOM that Windows Notepad may add
        if (source.rfind("\xEF\xBB\xBF", 0) == 0) source = source.substr(3);

        // Phase 1: lexer
        Lexer lexer(source);
        vector<Token> tokens = lexer.tokenize();

        // Phase 2: parser
        Parser parser(tokens);
        ProgramNode* program = parser.parse();
        if (program == nullptr || syntaxErrors > 0) {
            cerr << "Compilation failed: " << syntaxErrors << " syntax error(s)." << endl;
            return 1;
        }

        // Phase 3: semantic analysis
        SemanticAnalyzer analyzer;
        int semanticErrors = analyzer.analyze(program);
        if (semanticErrors > 0) {
            cerr << "Compilation failed: " << semanticErrors << " semantic error(s)." << endl;
            return 1;
        }

        // Phase 4: code generation
        CodeGenerator generator;
        string code = generator.generate(program);
        ofstream out(outputFile);
        if (!out) {
            cerr << "Cannot write file: " << outputFile << endl;
            return 1;
        }
        out << code;
        cout << "Compilation successful. Python code written to " << outputFile << endl;
    } catch (...) {
        cerr << "Compilation failed: unexpected internal error." << endl;
        return 1;
    }
    return 0;
}
