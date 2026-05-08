#include "Stmt.h"
#include <vector>

// AST root containing top-level statements.
class Program : public ASTNode {
public:
  std::vector<std::unique_ptr<Stmt>> stmt;
  
  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "Program:\n";
    for(const auto& stm : stmt) {
      stm->print(indent+2);
    }
  }
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    for(const auto& stm : stmt) {
      stm->analyzeAst(symTable);
    }
    return Type::VOID;
  }

  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
	for(const auto& stm : stmt) {
	  stm->Emit(codeGen);
	}
  }
};
