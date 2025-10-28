#include "program.h"
#include "lexer.h"

class Parser {
public:
  Parser(std::vector<Token> tok) : toks(tok) {}
  
  Token consumeToken(TokenType type, const std::string& value = "") {
    Token tok = peek();
    if(tok.type != type)
      throw std::runtime_error("Unexpected token type: " + (int)tok.type);
    if(!value.empty() && tok.value != value)
      throw std::runtime_error("Unexpected token value: " + tok.value);
    return consume();
  }

  Token consume() {
    return toks[pos++];
  }

  Token peek() {
    return toks[pos];
  }

  std::unique_ptr<Stmt> parseVarDecl() {
    consume(); // consume 'let'

    // variable name
    string name = consume().value;

    // colon
    if (consume().value != ":")
      throw std::runtime_error("Expected ':' in variable declaration");

    // type
    string typeName = consume().value;

    // equals
    if (consume().value != "=")
      throw std::runtime_error("Expected '=' in variable declaration");

    auto value = parseExpression();

    return std::make_unique<VarDeclStmt>(name, typeName, std::move(value));
  }

 
  std::unique_ptr<Program> parseProgram() {
    auto program = std::make_unique<Program>();

    // Keep parsing statements until the end-of-file token
    while (peek().type != TokenType::END) {
        program->stmt.push_back(parseStatement());
    }

    return program;
  }
  
  std::unique_ptr<Stmt> parseStatement() {
    if (peek().type == TokenType::KEYWORD && peek().value == "let") {
        return parseVarDecl();
    }

    // For now, fallback: treat any expression as an expression statement
    auto expr = parseExpression();
    return std::make_unique<ExprStmt>(std::move(expr));
  }

  std::unique_ptr<Expr> parseTerm() {
    auto node = parseFactor();
    while(peek().type == TokenType::OPERATOR &&
          (peek().value == "*" || peek().value == "/")) {
      char op = consume().value[0];
      auto right = parseFactor();
      node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(right));
    }
    return node;
  }
    
  std::unique_ptr<Expr> parseExpression() {
    auto node = parseTerm();
    while(peek().type == TokenType::OPERATOR &&
          (peek().value == "+" || peek().value == "-")) {
      char op = consume().value[0];
      auto right = parseTerm();
      node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(right));
    }
    return node;
  }

  std::unique_ptr<Expr> parseFactor() {
    Token tok = peek();
    switch(tok.type) {
      case TokenType::LITERAL:
        consume();
        if(isNumber(tok.value))
          return std::make_unique<NumberExpr>(std::stod(tok.value));
        else
          return std::make_unique<StringExpr>(tok.value);
      case TokenType::IDENTIFIER:
        consume();
        return std::make_unique<VariableExpr>(tok.value);
      case TokenType::PUNCTUATION:
        if(tok.value == "(")
        {
          consume();
          auto expr = parseExpression();
          consumeToken(TokenType::PUNCTUATION, ")");
          return expr;
        }
      default:
        throw std::runtime_error("Unexpected token in factor");
    }
  }


private:
  int pos = 0;
  std::vector<Token> toks;

  bool isNumber(std::string& s) {
    for (char c : s) {
      if (!isdigit(c)) {
        return false;
      }
    }
    return true;
  }
};
