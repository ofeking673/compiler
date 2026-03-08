#include <string>
#include <unordered_map>
#include <memory>


enum Type {
    INT,
    FLOAT,
    STRING,
    BOOL,
    VOID
};

struct Symbol {
    std::string name;
    Type type;
    bool isInitialized;
    bool isFunction;
    std::vector<Type> paramTypes;

};

class SymbolTable {
public:
  std::unordered_map<std::string, Symbol> symbolTable;
  std::shared_ptr<SymbolTable> parent;

  SymbolTable(std::shared_ptr<SymbolTable> p = nullptr) : parent(p) {}

  bool insert(const Symbol& sym) {
    if (symbolTable.find(sym.name) != symbolTable.end()) {
      return false; // Symbol already exists
    }
    symbolTable[sym.name] = sym;
    return true;
  }

  Symbol* lookup(const std::string& name) {
    if (symbolTable.find(name) != symbolTable.end())
      return &symbolTable[name];
    else if (parent)
      return parent->lookup(name);
    return nullptr; // Not found
  }
};

