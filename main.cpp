#include "lexer.h"
#include "fileIO.h"
#include "Parser.h"

int main() {
  FileIO file;
  string source = file.readFile("src.jas");
  
  Lexer lexer(source);
  std::vector<Token> tokens;

  while (true) {
    Token tok = lexer.next();
    std::cout << tok.value << std::endl;
    tokens.push_back(tok);

    if (tok.type == TokenType::END)
       break;
  }
  
  Parser parser(tokens);
  
  auto program = parser.parseProgram();
  program->print();

  return 0;
}

