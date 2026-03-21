#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <cctype>

using string = std::string;

enum class TokenType {
  KEYWORD,
  IDENTIFIER,
  TYPE_IDENTIFIER,
  LITERAL,
  OPERATOR,
  PUNCTUATION,
  END
};

std::string getTokenTypeName(TokenType type) {
  switch (type) {
    case TokenType::KEYWORD:
      return "Keyword";
    case TokenType::IDENTIFIER:
      return "Identifier";
    case TokenType::TYPE_IDENTIFIER:
      return "Type Identifier";
    case TokenType::LITERAL:
      return "Literal";
    case TokenType::OPERATOR:
      return "Operator";
    case TokenType::PUNCTUATION:
      return "Punctuation";
    case TokenType::END:
      return "EOF";
    default:
      return "INVALID";
  }
}

struct Token {
  TokenType type;
  string value;

  int line = 0;
  int column = 0;
};

class Lexer {
public:
  // Initialize the lexer source string (This will be read from the file)
  Lexer(string s) : src(move(s)) {}

  Token next() {
    skipWhitespace();
    char c = peek();

    if (c == '\0')
      return {TokenType::END, "", line, column};

      // Identifier / keyword / type
    if (isalpha(c) || c == '_') {
      string word;
      while (isalnum(peek()) || peek() == '_') word += get();

      if (keywords.count(word)) return {TokenType::KEYWORD, word, line, column};
      if (type_identifiers.count(word)) return {TokenType::TYPE_IDENTIFIER, word, line, column};
      if (literals.count(word)) return {TokenType::LITERAL, word, line, column};
      return {TokenType::IDENTIFIER, word, line, column};
    }

        // Numeric literal
    if (isdigit(c)) {
        string num;
        while (isdigit(peek())) num += get();
        // Check for range operator ".."
        if (peek() == '.' && src.substr(pos, 2) == "..") {
            // Do not consume '.'; let next() handle ".."
            return { TokenType::LITERAL, num, line, column};
        }
        // Check for decimal point
        if (peek() == '.') {
            get(); // consume '.'
            // If next is also '.', it's not a decimal, it's a range
            if (peek() == '.') {
                // Undo consuming '.'
                pos--;
                return { TokenType::LITERAL, num, line, column};
            }
            num += '.';
            while (isdigit(peek())) num += get();
        }
        return { TokenType::LITERAL, num, line, column};
    }


        // String literal (optional)
    if (c == '"') {
      get(); // skip "
      string str;
      while (peek() != '"' && peek() != '\0') str += get();
        get(); // skip closing "
          return {TokenType::LITERAL, str, line, column };
    }

        // Operators
    string twoCharOps[] = {"==", "!=", "->", ">=", "<=", "&&", "||", ".."};
    for (auto &op : twoCharOps) {
      if (src.substr(pos, op.size()) == op) {
        pos += op.size();
        return {TokenType::OPERATOR, op, line, column};
      }
    }

    // Single-char operator or punctuation
    string operators = "+-*/%=<>!";
    string punctuations = "(){},;:[]";

    if (operators.find(c) != string::npos) {
        get();
        return {TokenType::OPERATOR, string(1, c), line, column};
    }
    if (punctuations.find(c) != string::npos) {
      get();
      return {TokenType::PUNCTUATION, string(1, c), line, column };
    }

    throw std::runtime_error("Unexpected character: " + std::to_string(c) + "\nLine: " + std::to_string(line) + " - Column: " + std::to_string(column));
  }

private:
  string src;
  size_t pos = 0;
  int line = 0;
  int column = 0;
  std::unordered_set<string> keywords = {"fn", "let", "for", "if", "while", "else", "return", "in", "while"};
  std::unordered_set<string> type_identifiers = {"num", "str", "bool"};
  std::unordered_set<string> literals = {"true", "false"};

  char peek() const { return pos < src.length() ? src[pos] : '\0'; }
  char get() { return pos < src.length() ? src[pos++] : '\0'; }
  
  void skipWhitespace() {
      while (isspace(peek()))
      {		
          if (peek() == '\n') {
			  line++;
			  column = 0;
		  } else {
			  column++;
		  }
          get();
      }
  }
};
