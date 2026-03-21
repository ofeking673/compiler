#include "Stmt.h"

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

	virtual void print(int indent=0) const override {
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

	virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
		// Allocate array based on size from codeGen.ToQbeType()
		std::string arrayVar = codeGen.newTempVar();
		codeGen.emitIndent(indent);
		int sizeofType = codeGen.toQbeSize(type);
		std::string qbeType = codeGen.toQbeType(type);
		codeGen.output << arrayVar << " =alloc" << qbeType << " " << sizeofType * size << "\n";
		codeGen.varMap[name] = arrayVar;

		// Emit code for initializer if present
		for (size_t i = 0; i < initializer.size(); i++) {
			initializer[i]->Emit(codeGen, indent);
			std::string indexVar = codeGen.newTempVar();
			codeGen.emitIndent(indent);
			codeGen.emitArithmetic(indexVar, arrayVar, std::to_string(i * sizeofType), "+");
			codeGen.emitIndent(indent);
			codeGen.output << "store"<< qbeType << " " << codeGen.lastValue << ", " << indexVar << "\n";
		}
	}
};

class ArrayAccessExpr : public Expr {
public:
	std::shared_ptr<Expr> arrayExpr;
	std::shared_ptr<Expr> indexExpr;

	Symbol arraySymbol;

	ArrayAccessExpr(std::shared_ptr<Expr> arrayExpr, std::shared_ptr<Expr> indexExpr)
		: arrayExpr(arrayExpr), indexExpr(indexExpr) {}

	virtual void print(int indent=0) const override {
		printIndent(indent);
		std::cout << "ArrayAccessExpr:\n";
		arrayExpr->print(indent + 2);
		indexExpr->print(indent + 2);
	}

	virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {

		Type indexType = indexExpr->analyzeAst(symTable);
		if (indexType != Type::NUM) {
			throw std::runtime_error("Type error: Array index must be of type num.");
		}

		Symbol* sym = symTable->lookup(dynamic_cast<VariableExpr*>(arrayExpr.get())->name);

		//Copy sym to arraySymbol
		arraySymbol = *sym;

		if (arraySymbol.isArray == false) {
			throw std::runtime_error("Type error: Attempting to index a non-array variable.");
		}

		return indexType;
	}

	virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {

		// arrayExpr should be an array variable, indexExpr should be a num expression
		std::string arrayName = arraySymbol.name; // Get the array variable name
		std::string arrayVar = codeGen.varMap[arrayName]; // Get the corresponding QBE variable for the array

		indexExpr->Emit(codeGen, indent);
		std::string indexVar = codeGen.lastValue;

		// Calculate size of each element, then do basePtr + size * idx
		int sizeofType = codeGen.toQbeSize(arraySymbol.type);
		std::string elementPtr = codeGen.newTempVar();

		codeGen.emitIndent(indent);
		std::string offsetVar = codeGen.newTempVar();
		codeGen.emitArithmetic(offsetVar, indexVar, std::to_string(sizeofType), "*");

		codeGen.emitIndent(indent);
		codeGen.emitArithmetic(elementPtr, arrayVar, offsetVar, "+");

		codeGen.lastValue = elementPtr;
	}

	virtual bool isAddress() const override { return true; }
};