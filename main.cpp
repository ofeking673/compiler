#include "QbeCodeGen.h"
#include "Parser.h"
#include "fileIO.h"

int main() {
  FileIO file;
  string source = file.readFile("C:\\Users\\User\\compiler\\src.fg");
  
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

  codeGen.produceFinalFile("test.ssa");

  return 0;
}

