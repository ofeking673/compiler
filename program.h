#include "Stmt.h"
#include <vector>

class Program : public ASTNode {
public:
  std::vector<std::unique_ptr<Stmt>> stmt;
  
  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "Program:\n";
    for(const auto& stm : stmt) {
      stm->print(indent+2);
    }
  }
};
