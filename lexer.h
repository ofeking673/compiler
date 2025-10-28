#pragma once
#include <iostream>
#include <string>
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
};

class Lexer {
public:
  // Initialize the lexer source string (This will be read from the file)
  Lexer(string s) : src(move(s)) {}

  Token next() {
    skipWhitespace();
    char c = peek();

    if (c == '\0')
      return {TokenType::END, ""};

      // Identifier / keyword / type
    if (isalpha(c) || c == '_') {
      string word;
      while (isalnum(peek()) || peek() == '_') word += get();

      if (keywords.count(word)) return {TokenType::KEYWORD, word};
      if (type_identifiers.count(word)) return {TokenType::TYPE_IDENTIFIER, word};
        return {TokenType::IDENTIFIER, word};
    }

        // Numeric literal
    if (isdigit(c)) {
      string num;
      while (isdigit(peek())) num += get();
        if (peek() == '.') {
          num += get(); // consume '.'
          while (isdigit(peek())) num += get();
       }
      return {TokenType::LITERAL, num};
    }


        // String literal (optional)
    if (c == '"') {
      get(); // skip "
      string str;
      while (peek() != '"' && peek() != '\0') str += get();
        get(); // skip closing "
          return {TokenType::LITERAL, str};
    }

        // Operators
    string twoCharOps[] = {"==", "!=", "->", ">=", "<=", "&&", "||"};
    for (auto &op : twoCharOps) {
      if (src.substr(pos, op.size()) == op) {
        pos += op.size();
        return {TokenType::OPERATOR, op};
      }
    }

    // Single-char operator or punctuation
    string operators = "+-*/%=<>!";
    string punctuations = "(){},;:[]";

    if (operators.find(c) != string::npos) {
        get();
        return {TokenType::OPERATOR, string(1, c)};
    }
    if (punctuations.find(c) != string::npos) {
      get();
      return {TokenType::PUNCTUATION, string(1, c)};
    }

    throw std::runtime_error(string("Unexpected character: ") + c);
  }

private:
  string src;
  size_t pos = 0;
  std::unordered_set<string> keywords = {"fn", "let", "for", "if", "while", "else", "return"};
  std::unordered_set<string> type_identifiers = {"int", "str", "bool", "float"};

  char peek() const { return pos < src.length() ? src[pos] : '\0'; }
  char get() { return pos < src.length() ? src[pos++] : '\0'; }
  
  void skipWhitespace() {
    while(isspace(peek())) get();
  }
};
