#include "lexer.h"
#include "fileIO.h"
#include "Parser.h"

int main() {
  FileIO file;
  string source = file.readFile("src.fg");
  
  Lexer lexer(source);
  std::vector<Token> tokens;

  while (true) {
    Token tok = lexer.next();
    tokens.push_back(tok);

    if (tok.type == TokenType::END)
       break;
  }
  Parser parser(tokens);
  
  auto program = parser.parseProgram();
  program->print();

  return 0;
}

