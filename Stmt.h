#include "Expr.h"
#include <memory>

class Stmt : public ASTNode {
};

class VarDeclStmt : public Stmt {
public:
  std::string name;
  std::string type;
  std::unique_ptr<Expr> value;

  VarDeclStmt(const std::string& n, const std::string& t, std::unique_ptr<Expr> val) :
    name(n), type(t), value(std::move(val)) {}

  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "VarDeclStmt(" << name << " : " << type << ")\n";
    if(value) {
      printIndent(indent+2);
      std::cout << "Value: \n";
      value->print(indent+4);
    }
  }
};

class ExprStmt : public Stmt {
public:
  std::unique_ptr<Expr> expr;

  ExprStmt(std::unique_ptr<Expr> exp) : expr(std::move(exp)) {}
  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "ExprStmt:\n";
    if(expr) expr->print(indent+2);
  }
};
