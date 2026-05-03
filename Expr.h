#pragma once
#include "ASTNode.h"
#include "Stmt.h"
#include <memory>

struct Parameter {
  std::string name;
  std::string type;

  Parameter(const std::string& n, const std::string& t) : name(n), type(t) {}
};

class Expr : public ASTNode {
public:
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override { return Type::VOID; }
  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {}
};

class NumberExpr : public Expr {
public:
  int value;
  NumberExpr(int val) : value(val) {}

  virtual void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "NumberExpr(" << value << ")\n";}

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    return Type::NUM;
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
	  codeGen.lastValue = std::to_string(value);
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

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
      codeGen.emitIndent(indent);

	  std::string temp = codeGen.newTempVar();
	  codeGen.emitStringAssignment(temp, value);
	  codeGen.lastValue = temp;
  }

  virtual bool isAddress() const override { return true; }
};

class BooleanExpr : public Expr {
public:
    bool value;
    BooleanExpr(bool val) : value(val) {}

    virtual void print(int indent = 0) const override {
		printIndent(indent);
		std::cout << "BooleanExpr(" << (value ? "true" : "false") << ")\n";
	}

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
		return Type::BOOL;
	}

	virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
		codeGen.lastValue = value ? "1" : "0";
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

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override 
  {
      codeGen.lastValue = (name[0] == '%' ? "" : "%") + name;
  }
};

class BinaryExpr : public Expr {
public:
  std::string op;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;

  BinaryExpr(std::string oper, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) :
      op(oper), left(std::move(lhs)), right(std::move(rhs)) {}

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
       std::cout << leftType << " " << rightType << "\n";
       std::cout << op << "\n";
      throw std::runtime_error("Type mismatch in binary expression");
    }
    
    if (op == "<" || op == ">" || op == "==" || op == "!=" ||
        op == "<=" || op == ">=") {
        if (leftType != Type::NUM && leftType != Type::CHAR) {
            throw std::runtime_error("Invalid types for comparison operator: " + op);
		}
        return Type::BOOL;
    }

    return leftType; // Assuming both sides are of the same type
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) {
    left->Emit(codeGen, indent);
	std::string leftReg = codeGen.lastValue;
	right->Emit(codeGen, indent);
    std::string rightReg = codeGen.lastValue;
    codeGen.emitIndent(indent);
    std::string temp = codeGen.newTempVar();
	
    if (op == "<" || op == ">" || op == "==" || op == "!=" ||
        op == "<=" || op == ">=")
    {
		codeGen.emitComparison(temp, leftReg, rightReg, op);
    }
    else {
        codeGen.emitArithmetic(temp, leftReg, rightReg, op);
    }

	codeGen.lastValue = temp;
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
        if(funcName == "printf") return AnalyzePrint(symTable);

        Symbol* sym = symTable->lookup(funcName);
        if (!sym || !sym->isFunction) {
            throw std::runtime_error("Undefined function: " + funcName);
        }

        if (args.size() != sym->paramTypes.size()) {
            throw std::runtime_error("Argument count mismatch in function call to " + funcName);
        }

        for (size_t i = 0; i < sym->paramTypes.size(); ++i) {
            Type argType = args[i]->analyzeAst(symTable);
            if (argType != sym->paramTypes[i]) {
                throw std::runtime_error("Argument type mismatch in function call to " + funcName);
            }
        }


        return sym->type;
    }

    virtual Type AnalyzePrint(std::shared_ptr<SymbolTable> symTable) {
      if (args.size() == 0) {
          throw std::runtime_error("printf requires at least a format string argument");
      }
      Type formatType = args[0]->analyzeAst(symTable);
      if (formatType != Type::STRING) {
          throw std::runtime_error("First argument to printf must be a string literal");
      }
      std::cout << args.size();
      for (size_t i = 0; i < args.size(); ++i) {
          Type argType = args[i]->analyzeAst(symTable);
          std::cout << "Arg " << i << " type: " << (int)argType << "\n";
          if (argType != Type::NUM && argType != Type::STRING && argType != Type::BOOL) {
              throw std::runtime_error("Unsupported argument type in printf: " + std::to_string((int)argType));
          }
      }
      return Type::NUM;
    }  

    virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
		std::vector<std::string> argRegs;
		for (const auto& arg : args) {
			arg->Emit(codeGen, indent);
			argRegs.push_back(codeGen.lastValue);
		}

        std::string temp = codeGen.newTempVar();
        codeGen.emitIndent(indent);
        if (funcName == "printf") {
          codeGen.emitPrintf(argRegs[0], std::vector<std::string>(argRegs.begin() + 1, argRegs.end()));
        }
        else { 
		      codeGen.emitFuncCall(funcName, argRegs, temp);
          codeGen.lastValue = temp;
        }
	}
};

class ArrayAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> arrayExpr;
    std::unique_ptr<Expr> indexExpr;

    ArrayShape shape;
    Type arrayType;

    ArrayAccessExpr(std::unique_ptr<Expr> arrayExpr, std::unique_ptr<Expr> indexExpr)
        : arrayExpr(std::move(arrayExpr)), indexExpr(std::move(indexExpr)) {}

    virtual void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ArrayAccessExpr:\n";
        arrayExpr->print(indent + 2);
        indexExpr->print(indent + 2);
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
        VariableExpr* baseVar = unwrapBaseVariable(arrayExpr.get());
        ArrayShape* sh = symTable->lookupArrayShape(baseVar->name);
        shape = sh ? *sh : ArrayShape{};

        Type indexType = indexExpr->analyzeAst(symTable);
        if (indexType != Type::NUM) {
            throw std::runtime_error("Type error: Array index must be of type num.");
        }


        Symbol* sym = symTable->lookup(baseVar->name);
        if (!sym) {
            throw std::runtime_error("Undefined variable: " + baseVar->name);
        }

        //Copy sym to arraySymbol
        arrayType = sym->type;

        std::cout << "ArrayAccessExpr: Found symbol " << sym->name << " of type " << sym->type << "\n";
        if (sym->isArray == false) {
            throw std::runtime_error("Type error: Attempting to index a non-array variable.");
        }

        return indexType;
    }

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        getAddress(codeGen, indent);
        std::string elementPtr = codeGen.lastValue;
        std::string temp = codeGen.newTempVar();

        switch (codeGen.toQbeType(arrayType)[0]) {
        case 'b':
            codeGen.loadByteValue(elementPtr, temp);
			break;
        case 'w':
            codeGen.loadWordValue(elementPtr, temp);
            break;
        case 'l':
            codeGen.loadLabelValue(elementPtr, temp);
			break;
        default: // How did you manage to do this..?
        	throw std::runtime_error("Unsupported array element type in code generation.");
        }

        codeGen.lastValue = temp;
    }

    VariableExpr* unwrapBaseVariable(Expr* e) {
        while (true) {
            if (auto v = dynamic_cast<VariableExpr*>(e)) return v;
            if (auto a = dynamic_cast<ArrayAccessExpr*>(e)) {
                e = a->arrayExpr.get();
                continue;
            }
            return nullptr; // something like (foo()+bar)[i] -> not supported unless you want it
        }
    }

    void getAddress(QbeCodeGen& codeGen, int indent = 0) {
        VariableExpr* baseVar = unwrapBaseVariable(arrayExpr.get());
        if (!baseVar) {
            throw std::runtime_error("Array access base must be a variable or array access");
        }

        std::string varName = baseVar->name;
        std::string arrayVar = codeGen.varMap[varName]; // base pointer to flat storage

        int sizeofType = codeGen.toQbeSize(arrayType);

        // If we are doing arr[i][j], then `arrayExpr` is itself an ArrayAccessExpr (the inner [i]).
        if (auto inner = dynamic_cast<ArrayAccessExpr*>(arrayExpr.get())) {
            // Emit i from inner->indexExpr
            inner->indexExpr->Emit(codeGen, indent);
            std::string iVar = codeGen.lastValue;

            // Emit j from this->indexExpr
            indexExpr->Emit(codeGen, indent);
            std::string jVar = codeGen.lastValue;

            // Get cols for this array (must be stored when declaring [[T, cols], rows])
            int cols = shape.cols;

            // flatIdx = i*cols + j
            std::string mulVar = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.emitArithmetic(mulVar, iVar, std::to_string(cols), "*");

            std::string flatIdx = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.emitArithmetic(flatIdx, mulVar, jVar, "+");

            // offset = flatIdx * sizeofType
            std::string offsetVar = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.emitArithmetic(offsetVar, flatIdx, std::to_string(sizeofType), "*");

            std::string extendedOffset = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.output << extendedOffset << " =l extsw " << offsetVar << "\n";

            std::string elementPtr = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.emitArithmetic(elementPtr, arrayVar, extendedOffset, "+", true);

            codeGen.lastValue = elementPtr;
            return;
        }

        // 1D case: arr[idx]
        indexExpr->Emit(codeGen, indent);
        std::string indexVar = codeGen.lastValue;

        std::string offsetVar = codeGen.newTempVar();
        codeGen.emitIndent(indent);
        codeGen.emitArithmetic(offsetVar, indexVar, std::to_string(sizeofType), "*");

        std::string extendedOffset = codeGen.newTempVar();
        codeGen.emitIndent(indent);
        codeGen.output << extendedOffset << " =l extsw " << offsetVar << "\n";

        std::string elementPtr = codeGen.newTempVar();
        codeGen.emitIndent(indent);
        codeGen.emitArithmetic(elementPtr, arrayVar, extendedOffset, "+", true);

        codeGen.lastValue = elementPtr;
    }
    virtual bool isAddress() const override { return true; }
};
