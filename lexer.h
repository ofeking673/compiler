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

struct Token {
  string value;
  Type type;
};

class Lexer {
public:
  // Initialize the lexer source string (This will be read from the file)
  Lexer(string s) : src(move(s));

  public std::vector<Token> Tokenize() {
    skipWhiteSpace();
    char c = peek();

    if(c == '\0') return {TokenType::END, ""};

    if(isalpha(c) || c == '_') {
      string word;
      while(isalnum(peek()) || peek() = "_") word += get();

      if(keywords.count(word)) return {TokenType::KEYWORD, word};
      if(type_identifiers.count(word)) return {TokenType::TYPE_IDENTIFIER, word};

      return {TokenType::IDENTIFIER, word};
    }

    if(isdigit(c)) {
      string num;
      while(isdigit(peek())) num += get();
      return {TokenType::LITERAL, num};
    }

    if(c == '"') {
      get(); // Skip "
      string str;
      while(peek() != '"' && peek() != '\0') str += get();
      get(); // Skip closing "
      return {TokenType::LITERAL, str};
    }
    
    string twoCharOps[] = {"==", "!=", "->", ">=", "<=", "&&", "||"};
      for (auto &op : twoCharOps) {
        if (src.substr(pos, op.size()) == op) {
            pos += op.size();
            return {TokenType::Operator, op};
      }
    }


    string operators = "+-*/%=<>!";
    string punctuations = "(){},;:[]";

    if (operators.find(c) != string::npos) {
      get();
      return {TokenType::Operator, string(1, c)};
    }
    if (punctuations.find(c) != string::npos) {
          get();
          return {TokenType::Punctuation, string(1, c)};
    }
  
    throw runtime_error(string("Unexpected character: ") + c);
  }  

private:
  string src;
  size_t pos = 0;
  std::unordered_set<string> keywords = {"fn", "let", "for", "if", "while", "else", "return"};
  std::unordered_set<string> type_identifiers = {"int", "str", "bool", "float"};

  char peek() const { return pos < str.length() ? str[pos] : '\0'; }
  char get() { return pos < str.length() ? str[pos++] : '\0'; }
  
  void skipWhiteSpace() {
    while(isspace(peek())) get();
  }
}
