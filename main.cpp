#include "lexer.h"

int main() {
  string src = R"(
    fn greet(name: str) -> str {
      return "Hello"
    }
  )";

  Lexer lexer(src);
  Token tok;
  while ((tok = lexer.next()).type != TokenType::END) {
    std::cout << "Token(" << (int)tok.type << ", \"" << tok.value << "\")\n";
  }
}
