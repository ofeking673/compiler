#include <iostream>

class ASTNode {
  public:
  virtual ~ASTNode() = default;
  virtual void print(int indent = 0) const = 0;

protected:
  void printIndent(int indent) const {
    std::cout << std::string(indent, ' ');
  }
};
