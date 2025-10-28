#include "ASTNode.h"
#include <memory>
class Expr : public ASTNode {
};

class NumberExpr : public Expr {
public:
  double value;
  NumberExpr(double val) : value(val) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "NumberExpr(" << value << ")\n";}
};

class StringExpr : public Expr {
public:
  std::string value;
  StringExpr(std::string val) : value(val) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "StringExpr(" << value << ")\n";}
};

class VariableExpr : public Expr {
public:
  std::string name;
  VariableExpr(std::string n) : name(n) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "VariableExpr(" << name << ")\n";}
};

class BinaryExpr : public Expr {
public:
  char op;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;

  BinaryExpr(char oper, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) :
    op(oper), left(std::move(lhs)), right(std::move(rhs)) { }

  virtual void print(int indent = 0) const override 
  {
    printIndent(indent);
    std::cout << "BinaryOp(" << op << ")\n"; 
    if(left) left->print(indent + 2);
    if(right) right->print(indent + 2);
  }
};
