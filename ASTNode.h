#pragma once
#include <iostream>
#include "SymbolTable.h"

class ASTNode {
  public:
  virtual ~ASTNode() = default;
  virtual void print(int indent = 0) const = 0;
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) = 0;
protected:
  void printIndent(int indent) const {
    std::cout << std::string(indent, ' ');
  }
};
