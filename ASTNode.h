#pragma once
#include <iostream>
#include "QbeCodeGen.h"

class ASTNode {
  public:
  virtual ~ASTNode() = default;
  virtual void print(int indent = 0) const = 0;
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) = 0;

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) = 0;

  virtual bool isAddress() const { return false; }
protected:
  void printIndent(int indent) const {
    std::cout << std::string(indent, ' ');
  }
};
