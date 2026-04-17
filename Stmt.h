#pragma once
#include "Expr.h"
#include <memory>
#include <vector>
#include <optional>

class Stmt : public ASTNode {
public:
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {return Type::VOID; };
  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {};
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

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
    expr->Emit(codeGen, indent);
    codeGen.emitIndent(indent);
    codeGen.emitReturn(codeGen.lastValue);
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
    if      (type == "num")  varType = Type::NUM;
    else if (type == "str")  varType = Type::STRING;
    else if (type == "bool") varType = Type::BOOL;
    else throw std::runtime_error("Unknown type: " + type);

    Type valueType = value->analyzeAst(symTable);
    if(varType != valueType) {
      throw std::runtime_error("Type mismatch in variable declaration of " + name);
    }

    Symbol sym{name, varType, true, false};
    if(!symTable->insert(sym)) {
      throw std::runtime_error("Variable already declared: " + name);
    }

    return varType;
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
      codeGen.emitIndent(indent);
      std::string qbeType = codeGen.toQbeType(stringToType(type));
      std::string qbeSize = std::to_string(codeGen.toQbeSize(stringToType(type)));
      value->Emit(codeGen, indent);
      std::string valueVar = codeGen.lastValue;
      // For pointer types, we need to store the value into the allocated space


      if (value->isAddress()) {
          if (qbeType == "l") {
              codeGen.emitIndent(indent);
			  codeGen.output << "%" << name << " =alloc" << qbeSize << "\n";
              codeGen.emitIndent(indent);
              std::string tempLoadVar = codeGen.newTempVar();
              codeGen.output << tempLoadVar << " =l loadl " << valueVar << "\n";
			  codeGen.output << "storel " << tempLoadVar << ", %" << name << "\n";
		  }
		  else {
			  codeGen.output << "%" << name << " =alloc" << qbeSize << "\n";
			  codeGen.emitIndent(indent);
			  codeGen.output << "store" << qbeType << " " << valueVar << ", %" << name << "\n";
		  }
      }
      else {
          codeGen.emitIndent(indent);
          codeGen.output << "%" << name << " =" << qbeType << " copy " << valueVar << "\n";
      }
  }
};

class codeBlock : public Expr {
public:
  std::vector<std::unique_ptr<Stmt>> stmts;

  void addStmt(std::unique_ptr<Stmt> stmt) {
    stmts.push_back(std::move(stmt));
  }

  void print(int indent=0) const {
    printIndent(indent);
    std::cout << "{\n";
    for(const auto& stmt: stmts) {
      stmt->print(indent+2);
    }
    printIndent(indent);
    std::cout << "}\n";
  }

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    for(const auto& stmt : stmts) {
      stmt->analyzeAst(symTable);
    }
    return Type::VOID;
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
	for(const auto& stmt : stmts) {
	  stmt->Emit(codeGen, indent+1);
	}
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
        printIndent(indent);
        std::cout << "Params: {\n";

        for (const auto par : params) {
            printIndent(indent + 2);
            std::cout << par.name << " [" + par.type + "]\n";
        }
        printIndent(indent);
        std::cout << "}\n";
        if (body) body->print(indent + 2);
    }

    void collectReturnStatements(ASTNode* node, std::vector<ReturnStmt*>& returns) {
        if (auto retStmt = dynamic_cast<ReturnStmt*>(node)) {
            returns.push_back(retStmt);
        }
        else if (auto block = dynamic_cast<codeBlock*>(node)) {
            for (const auto& stmt : block->stmts) {
                collectReturnStatements(stmt.get(), returns);
            }
        }
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {

        // Create a new symbol table for the function scope
        auto funcSymTable = symTable->createChild();

        std::vector<Type> paramTypes;
        // Insert parameters into the function's symbol table
        for (const auto& param : params) {
            Type paramType = stringToType(param.type);
            paramTypes.push_back(paramType);

            Symbol sym{ param.name, paramType, true, false };
            if (!funcSymTable->insert(sym)) {
                throw std::runtime_error("Parameter already declared: " + param.name);
            }
        }

        // Analyze function body
        for (const auto& stmt : body->stmts) {
            stmt->analyzeAst(funcSymTable);
        }

        // Determine expected return type
        Type expectedType = stringToType(returnType);;
        std::vector<ReturnStmt*> returnStmts;
        collectReturnStatements(body.get(), returnStmts);

        for (auto* returnStmt : returnStmts) {
            Type retType = returnStmt->analyzeAst(funcSymTable);

            if (retType != expectedType) {
                throw std::runtime_error("Return type mismatch in function " + name);
            }
        }

        // Insert function symbol into the parent symbol table
        if (!symTable->insert({ name, expectedType, true, true, paramTypes })) {
            throw std::runtime_error("Function already declared: " + name);
        }

        return expectedType;
    }

    virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
		    std::vector<std::pair<Type, std::string>> qbeParams;
        for (const auto& param : params) {
            qbeParams.push_back({ stringToType(param.type), param.name });
        }
        codeGen.emitFunctionStart(name, stringToType(returnType), qbeParams);
        body->Emit(codeGen, indent+1);
        codeGen.emitFunctionEnd();
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

  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
	return expr->analyzeAst(symTable);
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
	codeGen.emitIndent(indent);
	expr->Emit(codeGen, indent);
  }
};

class AssignStmt : public Stmt {
public:
    std::string name; // Variable name being assigned
    std::unique_ptr<Expr> value; // Expression assigned
    Type type = Type::VOID;

    AssignStmt(const std::string& name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "AssignStmt: " + name + " <-\n";
        value.get()->print(indent+2);
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
		Symbol* sym = symTable->lookup(name);
		if(!sym) {
			throw std::runtime_error("Undefined variable: " + name);
		}
		Type valueType = value->analyzeAst(symTable);
		if(sym->type != valueType) {
			throw std::runtime_error("Type mismatch in assignment to " + name);
		}
        type = sym->type; // Store the variable type for code generation
		sym->isInitialized = true; // Mark variable as initialized
		return Type::VOID;
	}

    virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
        value->Emit(codeGen, indent); // Emit the expression, result in codeGen.lastValue

        // Determine QBE type (could use a helper or store type info)
        std::string qbeType = codeGen.toQbeType(type);

        // Emit assignment to the variable
        codeGen.emitIndent(indent);

        if (qbeType == "l") // Incase of pointer manipulation
        {
			std::string tempVar = codeGen.newTempVar();
            codeGen.output << tempVar << " =l loadl " << codeGen.lastValue << "\n";
            codeGen.emitIndent(indent);
            codeGen.output << "storel " << tempVar << ", " << "%" << name << "\n";
        }
        else
        {
            codeGen.output << "%" << name << " =" << qbeType << " copy " << codeGen.lastValue << "\n";
        }
        codeGen.lastValue = "%" + name;
    }
};

class ArrayAssignStmt : public Stmt {
public:
	std::string arrayName;
	std::shared_ptr<Expr> index;
	std::unique_ptr<Expr> value;

	ArrayAssignStmt(const std::string& name, std::shared_ptr<Expr> idx, std::unique_ptr<Expr> val) :
		arrayName(name), index(idx), value(std::move(val)) {}

	void print(int indent = 0) const override {
		printIndent(indent);
		std::cout << "ArrayAssignStmt: " + arrayName + "[\n";
		index->print(indent + 2);
		printIndent(indent);
		std::cout << "] <-\n";
		value->print(indent + 2);
	}

	virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
		Symbol* sym = symTable->lookup(arrayName);
		if (!sym) {
			throw std::runtime_error("Undefined variable: " + arrayName);
		}
		Type indexType = index->analyzeAst(symTable);
		if (indexType != Type::NUM) {
			throw std::runtime_error("Array index must be of type num");
		}
		Type valueType = value->analyzeAst(symTable);
		if (valueType != sym->type) {
			throw std::runtime_error("Type mismatch in array assignment to " + arrayName);
		}
		return Type::VOID;
	}

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        // Emit code to evaluate index and value
        index->Emit(codeGen, indent);
        auto indexVar = codeGen.lastValue;
        value->Emit(codeGen, indent);
        auto valueVar = codeGen.lastValue;

        
        ArrayAccessExpr arrayAccess = ArrayAccessExpr(std::make_shared<VariableExpr>(arrayName), std::make_shared<VariableExpr>(indexVar));
        arrayAccess.getAddress(codeGen, indent);
        auto elementAddr = codeGen.lastValue;
        Type type = arrayAccess.arrayType;

        std::string qbeType = codeGen.toQbeType(type);
        codeGen.emitIndent(indent);
        switch (qbeType[0]) {
            case 'l':
			    codeGen.storeLabelValue(valueVar, elementAddr);
			    break;
		    case 'w':
                codeGen.storeWordValue(valueVar, elementAddr);
                break;
            case 'b':
			    codeGen.storeByteValue(valueVar, elementAddr);
			    break;
            default: // How did we get here..?
                throw std::runtime_error("Unsupported type in array assignment");
        }

        codeGen.lastValue = valueVar;
    }
};

class ForLoopStmt : public Stmt {
public:
    std::string varName;
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Expr> body;

    ForLoopStmt(const std::string& var, std::unique_ptr<Expr> s, std::unique_ptr<Expr> e, std::unique_ptr<Expr> b) :
        varName(var), start(std::move(s)), end(std::move(e)), body(std::move(b)) {}

    virtual void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ForLoop(" << varName << ")\n";
        if (start) start->print(indent + 2);
        if (end) end->print(indent + 2);
        if (body) body->print(indent + 2);
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
        Type startType = start->analyzeAst(symTable);
        Type endType = end->analyzeAst(symTable);

        if (startType != Type::NUM || endType != Type::NUM) {
            throw std::runtime_error("For loop bounds must be numeric");
        }

        Symbol iterSymbol = { varName, Type::NUM, true, false };

        symTable->insert(iterSymbol);
            
        body->analyzeAst(symTable);

        return Type::VOID;
    }

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        std::string loopLabel = codeGen.newLabel("for_loop");
        std::string bodyLabel = codeGen.newLabel("for_body");
        std::string endLabel = codeGen.newLabel("for_end");

        // Evaluate start and end
        start->Emit(codeGen);
        auto startVar = codeGen.lastValue;
        end->Emit(codeGen);
        auto endVar = codeGen.lastValue;

        // Initialize loop variable
        
        codeGen.emitIndent(indent);
        codeGen.output << "%" << varName << " =w copy " << startVar << "\n";
        codeGen.emitIndent(indent);
        codeGen.output << "jmp " << loopLabel << "\n";

        codeGen.emitIndent(indent);
        codeGen.output << loopLabel << "\n";
        // Check loop condition
        codeGen.emitIndent(indent+1);
        std::string cond = codeGen.newTempVar();
        codeGen.emitComparison(cond ,"%" + varName, endVar, "<=");
        codeGen.output << "jnz " << cond << ", " << bodyLabel << ", " << endLabel << "\n";
        
        codeGen.emitIndent(indent);
        codeGen.output << bodyLabel << "\n";
        body->Emit(codeGen, indent);
        // Increment loop variable
        codeGen.emitIndent(indent+1);
        codeGen.output << "%" << varName << " =w add %" << varName << ", 1\n";
        codeGen.emitIndent(indent+1);
        codeGen.output << "jmp " << loopLabel << "\n";
        codeGen.emitIndent(indent);
        codeGen.output << endLabel << "\n";
    }
};

class WhileLoopStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
	std::unique_ptr<codeBlock> body;

	WhileLoopStmt(std::unique_ptr<Expr> cond, std::unique_ptr<codeBlock> b) :
		condition(std::move(cond)), body(std::move(b)) {}

	virtual void print(int indent = 0) const override {
		printIndent(indent);
		std::cout << "WhileLoop\n";
		if (condition) condition->print(indent + 2);
		if (body) body->print(indent + 2);
	}

	virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
		Type condType = condition->analyzeAst(symTable);
		if (condType != Type::BOOL) {
			throw std::runtime_error("While loop condition must be boolean");
		}
		body->analyzeAst(symTable);
		return Type::VOID;
	}

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        // Generate labels for loop start and end
        std::string loopLabel = codeGen.newLabel("while_loop");
        std::string endLabel = codeGen.newLabel("while_end");

        // Emit loop start label
        codeGen.emitIndent(indent);
        codeGen.output << loopLabel << "\n";
        // Emit condition
        condition->Emit(codeGen, indent+1);
        auto condVar = codeGen.lastValue;

        // Emit jump to end if condition is false
        codeGen.emitIndent(indent+1);
        codeGen.output << "jz " << condVar << ", " << endLabel << "\n";

        // Emit loop body
        body->Emit(codeGen, indent);
        // Emit jump back to loop start
        codeGen.emitIndent(indent+1);
        codeGen.output << "jmp " << loopLabel << "\n";
        // Emit loop end label
        codeGen.emitIndent(indent);
        codeGen.output << endLabel << "\n";
    }
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
	std::unique_ptr<codeBlock> thenBlock;
	std::unique_ptr<codeBlock> elseBlock;

	IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<codeBlock> thenB, std::unique_ptr<codeBlock> elseB = nullptr) :
		condition(std::move(cond)), thenBlock(std::move(thenB)), elseBlock(std::move(elseB)) {}

	virtual void print(int indent = 0) const override {
		printIndent(indent);
		std::cout << "IfStmt\n";
		if (condition) condition->print(indent + 2);
		if (thenBlock) thenBlock->print(indent + 2);
		if (elseBlock) elseBlock->print(indent + 2);
	}

	virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
		Type condType = condition->analyzeAst(symTable);
		if (condType != Type::BOOL) {
			throw std::runtime_error("If statement condition must be boolean");
		}
        auto thenSymTable = symTable->createChild();
		thenBlock->analyzeAst(thenSymTable);
		if (elseBlock) {
            auto elseSymTable = symTable->createChild();
			elseBlock->analyzeAst(elseSymTable);
		}
		return Type::VOID;
	}

	virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        std::string thenLabel = codeGen.newLabel("then");
        std::string elseLabel = codeGen.newLabel("else");
        std::string endifLabel = codeGen.newLabel("endif");

        condition->Emit(codeGen, indent);
        auto condVar = codeGen.lastValue;

        codeGen.emitIndent(indent);
        codeGen.output << "jnz " << condVar << ", " << thenLabel << ", " << elseLabel << "\n";
        codeGen.emitIndent(indent);
        codeGen.output << thenLabel << "\n";
        thenBlock->Emit(codeGen, indent);
        codeGen.emitIndent(indent + 1);
        codeGen.output << "jmp " << endifLabel << "\n";

        codeGen.emitIndent(indent);
        codeGen.output << elseLabel << "\n";

        if (elseBlock) {
			elseBlock->Emit(codeGen, indent);
		}
        codeGen.emitIndent(indent+1);
        codeGen.output << "jmp " << endifLabel << "\n";

        codeGen.emitIndent(indent);
        codeGen.output << endifLabel << "\n";
	}
};

class ImportStmt : public Stmt {
public:
    std::string moduleName;

    ImportStmt(const std::string& name) : moduleName(name) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ImportStmt: " << moduleName << "\n";
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
        
        return Type::VOID;
    }

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {

    }
};

class ArrayDeclStmt : public Stmt {
public:
    std::string name;
    Type type;
    int size;
    std::vector<std::shared_ptr<Expr>> initializer;

    ArrayDeclStmt(const std::string& name,
        Type type,
        int size = -1,
        std::vector<std::shared_ptr<Expr>> init = {})
        : name(name), type(type), size(size), initializer(init) {}

    virtual void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ArrayDeclStmt: " << type << " " << name << "[" << size << "]" << std::endl;
    }

    virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
        if (!CheckAllVarTypes(symTable))
        {
            throw new std::runtime_error("Type error in array initializer");
        }

        if (size == -1) {
            size = initializer.size();
        }

        Symbol arraySymbol = { name, type, true, false, {}, true };
        symTable->insert(arraySymbol);
        return Type::ARRAY;
    }

    bool CheckAllVarTypes(std::shared_ptr<SymbolTable> symTable) {
        for (const auto& expr : initializer) {
            Type exprType = expr->analyzeAst(symTable); // Pass symbol table if needed
            if (exprType != type) {
                std::cerr << "Type error: Array initializer type does not match declared type.\n";
                return false;
            }
        }
        return true;
    }

    virtual void Emit(QbeCodeGen& codeGen, int indent = 0) override {
        // Allocate array based on size from codeGen.ToQbeType()
        std::string arrayVar = codeGen.newTempVar();
        codeGen.emitIndent(indent);
        int sizeofType = codeGen.toQbeSize(type);
        std::string qbeType = codeGen.toQbeType(type);
        codeGen.output << arrayVar << " =l alloc" << sizeofType << " " << sizeofType * size << "\n";
        codeGen.varMap[name] = arrayVar;

        // Emit code for initializer if present
        for (size_t i = 0; i < initializer.size(); i++) {
            initializer[i]->Emit(codeGen, indent);
            std::string indexVar = codeGen.newTempVar();
            codeGen.emitIndent(indent);
            codeGen.emitArithmetic(indexVar, arrayVar, std::to_string(i * sizeofType), "+", true);
            codeGen.emitIndent(indent);
            codeGen.output << "store" << qbeType << " " << codeGen.lastValue << ", " << indexVar << "\n";
        }
    }
};