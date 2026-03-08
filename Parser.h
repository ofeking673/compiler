#include "program.h"
#include "lexer.h"

class Parser {
public:
  Parser(std::vector<Token> tok) : toks(tok) {}
  
  Token consumeToken(TokenType type, const std::string& value = "") {
    Token tok = peek();
    if(tok.type != type)
      throw std::runtime_error("Unexpected token type: " + std::to_string(static_cast<int>(tok.type)) + "\nExpected was: " + std::to_string((int)type) + "\nValue was: " + tok.value);
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

    string name = consume().value;

    if (consume().value != ":")
      throw std::runtime_error("Expected ':' in variable declaration");

    // type
    string typeName = consume().value;

    if (consume().value != "=")
      throw std::runtime_error("Expected '=' in variable declaration");

    auto value = parseExpression();

    return std::make_unique<VarDeclStmt>(name, typeName, std::move(value));
  }

 
  std::unique_ptr<Program> parseProgram() {
    auto program = std::make_unique<Program>();

    while (peek().type != TokenType::END) {
      auto stmt = parseStatement();  
      if(stmt) program->stmt.push_back(std::move(stmt));
    }

    return program;
  }
  
  std::unique_ptr<Stmt> parseStatement() {
    if (peek().type == TokenType::END) 
      return nullptr;

    if (peek().type == TokenType::KEYWORD && peek().value == "let") {
        return parseVarDecl();
    }
    else if (peek().type == TokenType::KEYWORD && peek().value == "fn") {
      return parseFunction();
    }
    else if (peek().type == TokenType::KEYWORD && peek().value == "return") {
      consume(); // consume 'return'
      auto expr = parseExpression();
      return std::make_unique<ReturnStmt>(std::move(expr));
    }
    else if (peek().type == TokenType::IDENTIFIER) {
        // Could be an expression statement (e.g., function call) or assignment
        auto expr = parseExpression();
        if (peek().type == TokenType::OPERATOR && peek().value == "=") {
            consume(); // consume '='
            auto value = parseExpression();
            if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
                return std::make_unique<AssignStmt>(varExpr->name, std::move(value));
            }
            else {
                throw std::runtime_error("Left-hand side of assignment must be a variable");
            }
        }

        // If it's not an assignment, treat it as an expression statement
        // Expr is an FuncCallExpr
        return std::make_unique<ExprStmt>(std::move(expr));
    }
    // For now, fallback: treat any expression as an expression statement
    auto expr = parseExpression();
    return std::make_unique<ExprStmt>(std::move(expr));
  }
  
  std::unique_ptr<Stmt> parseFunction() {
    consumeToken(TokenType::KEYWORD, "fn");
    std::string funcName = consume().value;
    
    consumeToken(TokenType::PUNCTUATION, "(");
    std::vector<Parameter> params;

    while(!(peek().type == TokenType::PUNCTUATION && peek().value == ")")) {
      std::string paramName = consume().value;
      consumeToken(TokenType::PUNCTUATION, ":");
      std::string paramType = consume().value;

      params.emplace_back(paramName, paramType);

      if(peek().type == TokenType::PUNCTUATION && peek().value == ",") {
        consume();
      }
      else {
        break;
      }
    }
    consumeToken(TokenType::PUNCTUATION, ")");
    
    // Parse return type
    consumeToken(TokenType::OPERATOR, "->");
    std::string returnType = consume().value;
    
    auto body = parseBlock();
    return std::make_unique<FuncDeclStmt>(funcName, params, returnType, std::move(body));
  }

  std::unique_ptr<codeBlock> parseBlock() {
    consumeToken(TokenType::PUNCTUATION, "{");
    
    auto block = std::make_unique<codeBlock>();

    while(!(peek().type == TokenType::PUNCTUATION && peek().value == "}")) {
      auto stmt = parseStatement();
      block->addStmt(std::move(stmt));
    }
    
    consumeToken(TokenType::PUNCTUATION, "}");
    return block;
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
    std::string name;
    switch(tok.type) {
      case TokenType::END:
        return nullptr; // For extreme cases of unclosed parenthesis, etc.
      case TokenType::LITERAL:
        consume();
        if(isNumber(tok.value))
          return std::make_unique<NumberExpr>(std::stod(tok.value));
        else
          return std::make_unique<StringExpr>(tok.value);
      case TokenType::IDENTIFIER:
          name = tok.value;
          consume();

          if (peek().type == TokenType::PUNCTUATION && peek().value == "(")
          {
            consume(); // consume '('
			std::vector<std::unique_ptr<Expr>> args;

			if (!(peek().type == TokenType::PUNCTUATION && peek().value == ")")) {
			  while (true) {
				args.push_back(parseExpression());
				if (peek().type == TokenType::PUNCTUATION && peek().value == ",") {
				  consume(); // consume ','
				} else {
				  break;
				}
			  }
			}
			consumeToken(TokenType::PUNCTUATION, ")");

			return std::make_unique<FuncCallExpr>(name, std::move(args));
          }
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
