#include "Expr.h"
#include <memory>
#include <optional>

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


struct codeBlock {
  std::vector<std::unique_ptr<Stmt>> stmts;

  void addStmt(std::unique_ptr<Stmt> stmt) {
    stmts.push_back(std::move(stmt));
  }

  void print(int indent=0) const {
    std::cout << "{\n";
    for(const auto& stmt: stmts) {
      stmt->print(indent);
    }
    std::cout << "}\n";
  }
};

class FuncDeclStmt : public Stmt {
public:
  std::string name;
  std::vector<Parameter> params;
  std::string returnType;
  std::unique_ptr<codeBlock> body;

  FuncDeclStmt(const std::string& n, const std::vector<Parameter>& par, const std::string& ret, std::unique_ptr<codeBlock> b = {}
               ) : name(n), params(par), returnType(ret), body(std::move(b)) {}

  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "FuncStmt (" + name + ", " + returnType + "):\n";
    if(body) body->print(indent+2);
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
