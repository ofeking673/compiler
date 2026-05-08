#include "Stmt.h"
#include <vector>

// AST root containing top-level statements.
class Program : public ASTNode {
public:
  std::vector<std::unique_ptr<Stmt>> stmt;
  
  // Summary: Pretty-prints the program AST.
  // Input: indentation level for display formatting.
  // Output: no return value; AST text written to stdout.
  void print(int indent = 0) const override {
    printIndent(indent);
    std::cout << "Program:\n";
    for(const auto& stm : stmt) {
      stm->print(indent+2);
    }
  }
  // Summary: Performs semantic analysis over all top-level statements.
  // Input: root symbol table for analysis scope.
  // Output: Type::VOID on success; may throw on semantic errors.
  virtual Type analyzeAst(std::shared_ptr<SymbolTable> symTable) override {
    for(const auto& stm : stmt) {
      stm->analyzeAst(symTable);
    }
    return Type::VOID;
  }

  // Summary: Emits QBE code for all top-level statements in order.
  // Input: mutable QbeCodeGen state/output collector and optional indentation.
  // Output: no return value; codegen output buffer is appended.
  virtual void Emit(QbeCodeGen& codeGen, int indent=0) override {
	for(const auto& stm : stmt) {
	  stm->Emit(codeGen);
	}
  }
};
