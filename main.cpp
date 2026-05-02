#include "QbeCodeGen.h"
#include "Parser.h"
#include "fileIO.h"
#include "compiler.h"

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage:\n"
            << "  compile file.fg    - Compile the source file\n"
            << "  compile --lib file.fg    - Compile the source file into a lib\n";
        return 1;
    }

    bool buildLib = false;
    std::string filePath;

    if (std::string(argv[1]) == "--lib") {
        if (argc < 3) {
            std::cerr << "Error: Missing file path for library compilation.\n";
            return 1;
        }
        buildLib = true;
        filePath = argv[2];
    }
    else {
        filePath = argv[1];
    }
  FileIO file;
  string source = file.readFile(filePath);
  if (source.empty()) {
	std::cerr << "Error: Could not read file " << filePath << std::endl;
	return 1;
  }
  
  Lexer lexer(source);
  std::vector<Token> tokens;

  while (true) {
    Token tok = lexer.next();
    tokens.push_back(tok);
    std::cout << (int)tok.type << " - " << tok.value << std::endl;
    if (tok.type == TokenType::END)
       break;
  }
  Parser parser(tokens);
  
  auto program = parser.parseProgram();

  try {
    program->analyzeAst(std::make_shared<SymbolTable>());
    std::cout << "Semantic analysis completed successfully." << std::endl;
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Semantic analysis failed: " << e.what() << std::endl;
    return 1;
  }
  program->print();

  QbeCodeGen codeGen;
  program->Emit(codeGen);

  compiler::processFile(filePath, buildLib, codeGen);
  return 0;
}
