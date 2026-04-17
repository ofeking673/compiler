#pragma once
#include "program.h"
#include "lexer.h"
#include "Stmt.h"
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
    if (typeName == "[")
    {
      return parseArrayDeclStmt(name);
    }

    if (consume().value != "=")
      throw std::runtime_error("Expected '=' in variable declaration");

    auto value = parseExpression();

    return std::make_unique<VarDeclStmt>(name, typeName, std::move(value));
  }
 
  std::unique_ptr<Program> parseProgram() {
    auto program = std::make_unique<Program>();

    while (peek().type != TokenType::END) {
      try {
        auto stmt = parseStatement();
        if (stmt) program->stmt.push_back(std::move(stmt));
      }
      catch (...) {
        std::cout << "Error parsing statement at token: " << peek().value << std::endl;
        std::cout << "Line: " << peek().line << ", Column: " << peek().column << std::endl;
        throw;
      }
    }

    return program;
  }
  
  std::unique_ptr<Stmt> parseArrayDeclStmt(std::string name) {
	// We are at the point where we have consumed 'let name : ['

	std::string elemType = consume().value;
    consumeToken(TokenType::PUNCTUATION, ",");
    std::string size = consume().value;

    consumeToken(TokenType::PUNCTUATION, "]");
	
	if (consume().value != "=")
	  throw std::runtime_error("Expected '=' in array declaration");

	std::vector<std::shared_ptr<Expr>> initializer;

	if (peek().type == TokenType::PUNCTUATION && peek().value == "{") {
		consume(); // consume '{'
		while (!(peek().type == TokenType::PUNCTUATION && peek().value == "}")) {
			initializer.push_back(std::move(parseExpression()));
			if (peek().type == TokenType::PUNCTUATION && peek().value == ",") {
				consume(); // consume ','
			}
			else {
				break;
			}
		}
		consumeToken(TokenType::PUNCTUATION, "}");
	}

	return std::make_unique<ArrayDeclStmt>(name, stringToType(elemType), -1, std::move(initializer));
  }

  std::unique_ptr<Stmt> parseStatement() {
    if (peek().type == TokenType::END) 
      return nullptr;
    if (peek().type == TokenType::IMPORT) {
      consume(); // consume 'import'
      std::string moduleName = consume().value;
      return std::make_unique<ImportStmt>(moduleName);
    }
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
    else if (peek().type == TokenType::KEYWORD && peek().value == "for") {
	  return parseForLoop();
	}
    else if (peek().type == TokenType::KEYWORD && peek().value == "while") {
	  return parseWhileLoop();
	}
    else if (peek().type == TokenType::KEYWORD && peek().value == "if")
    {
      return parseIfStmt();
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
            else if (auto arrayAccessExpr = dynamic_cast<ArrayAccessExpr*>(expr.get())) {
				// Handle array element assignment
				auto varExpr = dynamic_cast<VariableExpr*>(arrayAccessExpr->arrayExpr.get());
				if (!varExpr) {
					throw std::runtime_error("Left-hand side of assignment must be a variable or array access");
				}

				return std::make_unique<ArrayAssignStmt>(varExpr->name, arrayAccessExpr->indexExpr, std::move(value));
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
      std::string op = consume().value;
      auto right = parseFactor();
      node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(right));
    }
    return node;
  }
  
  std::unique_ptr<Stmt> parseForLoop() {
    consumeToken(TokenType::KEYWORD, "for");
	std::string varName = consume().value;
	consumeToken(TokenType::KEYWORD, "in");
	auto op = parseExpression(); // This returns a binary Expr containing op = ..
    auto* binOp = dynamic_cast<BinaryExpr*>(op.get());
    
    auto s = std::move(binOp->left);
    auto e = std::move(binOp->right);
    

	auto body = parseBlock();
	return std::make_unique<ForLoopStmt>(varName, std::move(s), std::move(e),std::move(body));
  }

  std::unique_ptr<Stmt> parseIfStmt() {
      consumeToken(TokenType::KEYWORD, "if");
      auto condition = parseExpression();
      auto thenBlock = parseBlock();

      std::unique_ptr<codeBlock> elseBlock = nullptr;
      if (peek().type == TokenType::KEYWORD && peek().value == "else") {
          consumeToken(TokenType::KEYWORD, "else");
          elseBlock = parseBlock();
      }

      return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
  }

  std::unique_ptr<Stmt> parseWhileLoop() {
    consumeToken(TokenType::KEYWORD, "while");
    auto condition = parseExpression();
    auto body = parseBlock();
    return std::make_unique<WhileLoopStmt>(std::move(condition), std::move(body));
  }

  std::unique_ptr<Expr> parseExpression() {
    auto node = parseComparisonExpr();
    while(peek().type == TokenType::OPERATOR &&
          (peek().value == "+" || peek().value == "-")) {
      std::string op = consume().value;
      auto right = parseComparisonExpr();
      node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(right));
    }
    return node;
  }

  std::unique_ptr<Expr> parseComparisonExpr() {
      auto node = parseRangeExpr();
      while (peek().type == TokenType::OPERATOR &&
          (peek().value == "==" || peek().value == "!=" || peek().value == "<" || peek().value == ">" || peek().value == "<=" || peek().value == ">=")) {
          std::string op = consume().value;
          auto right = parseRangeExpr();
          node = std::make_unique<BinaryExpr>(op, std::move(node), std::move(right));
      }
      return node;
  }

  std::unique_ptr<Expr> parseRangeExpr() {
      auto node = parseTerm();
      while (peek().type == TokenType::OPERATOR && peek().value == "..") {
          std::string op = consume().value;
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
        if (isNumber(tok.value))
            return std::make_unique<NumberExpr>(std::stoi(tok.value));
        else if (tok.value == "true" || tok.value == "false")
            return std::make_unique<BooleanExpr>(tok.value == "true");
        else
          return std::make_unique<StringExpr>(tok.value);
      case TokenType::IDENTIFIER: // Case for variables and function calls
        return parseIdentCall(tok);
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

  std::unique_ptr<Expr> parseIdentCall(Token tok) {
      std::string name = tok.value;
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
                  }
                  else {
                      break;
                  }
              }
          }
          consumeToken(TokenType::PUNCTUATION, ")");

          return std::make_unique<FuncCallExpr>(name, std::move(args));
      }

      if (peek().type == TokenType::PUNCTUATION && peek().value == "[")
      {
          consumeToken(TokenType::PUNCTUATION, "[");
          auto indexExpr = parseExpression();
          consumeToken(TokenType::PUNCTUATION, "]");
          return std::make_unique<ArrayAccessExpr>(std::make_unique<VariableExpr>(name), std::move(indexExpr));
      }

	  return std::make_unique<VariableExpr>(name);
  }

private:
  int pos = 0;
  std::vector<Token> toks;

  bool isNumber(string myString) {
      std::istringstream iss(myString);
      float f;
      iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
      // Check the entire string was consumed and if either failbit or badbit is set
      return iss.eof() && !iss.fail();
  }
};
