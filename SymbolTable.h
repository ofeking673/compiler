#include <string>
#include <map>
#include <memory>


enum Type {
    NUM,
    STRING,
    BOOL,
    CHAR,
    VOID,
    ARRAY,
};

Type stringToType(const std::string& typeStr) {
	if (typeStr == "num") return Type::NUM;
	else if (typeStr == "str") return Type::STRING;
	else if (typeStr == "bool") return Type::BOOL;
	else if (typeStr == "char") return Type::CHAR;
	else if (typeStr == "void") return Type::VOID;
	else if (typeStr == "array") return Type::ARRAY;
	throw std::invalid_argument("Unknown type: " + typeStr);
}

struct Symbol {
    std::string name;
    Type type;
    bool isInitialized = false;
    bool isFunction = false;
    std::vector<Type> paramTypes = {};
    bool isArray = false;
};

class SymbolTable {
public:
  std::map<std::string, Symbol> symbolTable;
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
      auto it = symbolTable.find(name);
      if (it != symbolTable.end())
          return &it->second;
      else if (parent)
          return parent->lookup(name);
      return nullptr;
  }

  std::shared_ptr<SymbolTable> createChild() {
	return std::make_shared<SymbolTable>(std::make_shared<SymbolTable>(*this));
  }
};