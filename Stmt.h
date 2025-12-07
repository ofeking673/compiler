#include "Expr.h"
#include <memory>
#include <vector>
#include <optional>

class Stmt : public ASTNode {
public:
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {return Type::VOID; };
};

class ReturnStmt : public Stmt {
public:
  std::unique_ptr<Expr> expr;
  ReturnStmt(std::unique_ptr<Expr> exp) : expr(std::move(exp)) {}

  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "ReturnStmt:\n";
    if(expr) expr->print(indent+2);
  }

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    return expr->analyzeAst(symTable);
  }
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

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    Type varType;
    if(type == "int") varType = Type::INT;
    else if(type == "float") varType = Type::FLOAT;
    else if(type == "str") varType = Type::STRING;
    else if(type == "bool") varType = Type::BOOL;
    else throw std::runtime_error("Unknown type: " + type);

    Type valueType = value->analyzeAst(symTable);
    if(varType != valueType) {
      throw std::runtime_error("Type mismatch in variable declaration of " + name);
    }

    Symbol sym{name, varType, true};
    if(!symTable->insert(sym)) {
      throw std::runtime_error("Variable already declared: " + name);
    }

    return varType;
  }
};


class codeBlock : public ASTNode {
public:
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

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    for(const auto& stmt : stmts) {
      stmt->analyzeAst(symTable);
    }
    return Type::VOID;
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
  
  void collectReturnStatements(ASTNode* node, std::vector<ReturnStmt*>& returns) {
    if(auto retStmt = dynamic_cast<ReturnStmt*>(node)) {
      returns.push_back(retStmt);
    }
    else if(auto block = dynamic_cast<codeBlock*>(node)) {
      for(const auto& stmt : block->stmts) {
        collectReturnStatements(stmt.get(), returns);
      }
    }
  }

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    // Create a new symbol table for the function scope
    auto funcSymTable = std::make_shared<SymbolTable>(symTable);

    // Insert parameters into the function's symbol table
    for(const auto& param : params) {
      Type paramType;
      if(param.type == "int") paramType = Type::INT;
      else if(param.type == "float") paramType = Type::FLOAT;
      else if(param.type == "str") paramType = Type::STRING;
      else if(param.type == "bool") paramType = Type::BOOL;
      else throw std::runtime_error("Unknown parameter type: " + param.type);

      Symbol sym{param.name, paramType, true};
      if(!funcSymTable->insert(sym)) {
        throw std::runtime_error("Parameter already declared: " + param.name);
      }
    }

    // Analyze the function body
    for(const auto& stmt : body->stmts) {
      stmt->analyzeAst(funcSymTable);
    }

    // Return type handling
    std::vector<ReturnStmt*> returnStmts;
    collectReturnStatements(body.get(), returnStmts);

    for(const auto* returnStmt : returnStmts) {
      Type retType = returnStmt->analyzeAst(funcSymTable);
      Type expectedType;
      if(returnType == "int") expectedType = Type::INT;
      else if(returnType == "float") expectedType = Type::FLOAT;
      else if(returnType == "str") expectedType = Type::STRING;
      else if(returnType == "bool") expectedType = Type::BOOL;
      else if(returnType == "void") expectedType = Type::VOID;
      else throw std::runtime_error("Unexpected return type: " + returnType);

      if(retType != expectedType) {
        throw std::runtime_error("Return type mismatch in function " + name);
      }
    }
    return Type::VOID; 
  }     // Assuming functions return void for now
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
