#include "ASTNode.h"
#include <memory>

struct Parameter {
  std::string name;
  std::string type;

  Parameter(const std::string& n, const std::string& t) : name(n), type(t) {}
};

class Expr : public ASTNode {
};

class NumberExpr : public Expr {
public:
  double value;
  NumberExpr(double val) : value(val) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "NumberExpr(" << value << ")\n";}

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    return Type::INT; // Assuming all numbers are ints
  }
};

class StringExpr : public Expr {
public:
  std::string value;
  StringExpr(std::string val) : value(val) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "StringExpr(" << value << ")\n";}

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    return Type::STRING;
  }
};

class VariableExpr : public Expr {
public:
  std::string name;
  VariableExpr(std::string n) : name(n) {}
  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "VariableExpr(" << name << ")\n"; }

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    Symbol* sym = symTable->lookup(name);
    if(!sym) {
      throw std::runtime_error("Undefined variable: " + name);
    }
    return sym->type;
  }
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

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    Type leftType = left->analyzeAst(symTable);
    Type rightType = right->analyzeAst(symTable);

    if(leftType != rightType) {
      throw std::runtime_error("Type mismatch in binary expression");
    }

    return leftType; // Assuming both sides are of the same type
  }
};

class FuncCallExpr : public Expr {
public:
    std::string funcName;
    std::vector<std::unique_ptr<Expr>> args;

    FuncCallExpr(const std::string& name, std::vector<std::unique_ptr<Expr>> arguments) :
        funcName(name), args(std::move(arguments)) {}

    virtual void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "FuncCall(" << funcName << ")\n";
        for (const auto& arg : args) {
            arg->print(indent + 2);
        }
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
        Symbol* sym = symTable->lookup(funcName);
        if (!sym || !sym->isFunction) {
            throw std::runtime_error("Undefined function: " + funcName);
        }

        if (args.size() != sym->paramTypes.size()) {
            throw std::runtime_error("Argument count mismatch in function call to " + funcName);
        }

        for (size_t i = 0; i < args.size(); ++i) {
            Type argType = args[i]->analyzeAst(symTable);
            if (argType != sym->paramTypes[i]) {
                throw std::runtime_error("Argument type mismatch in function call to " + funcName);
            }
        }


        return sym->type;
    }
};