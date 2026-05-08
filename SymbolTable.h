#include <string>
#include <map>
#include <memory>
#include <vector>

// Semantic types supported by the language.
enum Type {
    NUM,
    STRING,
    BOOL,
    CHAR,
    VOID,
    ARRAY,
};

// Converts source-level type names to semantic enum values.
// Summary: Converts parser type text to internal enum.
// Input: source-level type string (e.g. "num", "str").
// Output: corresponding Type enum value, or throws for unknown input.
Type stringToType(const std::string& typeStr) {
	if (typeStr == "num") return Type::NUM;
	else if (typeStr == "str") return Type::STRING;
	else if (typeStr == "bool") return Type::BOOL;
	else if (typeStr == "char") return Type::CHAR;
	else if (typeStr == "void") return Type::VOID;
	else if (typeStr == "array") return Type::ARRAY;
	throw std::invalid_argument("Unknown type: " + typeStr);
}

// Symbol metadata tracked during semantic analysis and codegen preparation.
struct Symbol {
    std::string name;
    Type type;
    bool isInitialized = false;
    bool isFunction = false;
    std::vector<Type> paramTypes = {};
    bool isArray = false;
};

// Shape metadata for 1D/2D arrays.
struct ArrayShape {
    int dims = 1;
    int rows = -1;
    int cols = -1;
};

// Hierarchical symbol table used for lexical scopes.
class SymbolTable {
public:
  std::map<std::string, Symbol> symbolTable;
  std::shared_ptr<SymbolTable> parent;

  std::unordered_map<std::string, ArrayShape> arrayShapes;

  SymbolTable(std::shared_ptr<SymbolTable> p = nullptr) : parent(p) {}

  // Summary: Inserts a symbol into the current scope only.
  // Input: symbol metadata to register.
  // Output: true on success, false when symbol already exists in this scope.
  bool insert(const Symbol& sym) {
    if (symbolTable.find(sym.name) != symbolTable.end()) {
      return false; // Symbol already exists
    }
    symbolTable[sym.name] = sym;
    return true;
  }
  
  // Summary: Resolves array shape metadata through current and parent scopes.
  // Input: symbol name.
  // Output: pointer to ArrayShape if found, otherwise nullptr.
  ArrayShape* lookupArrayShapes(const std::string& name) {
        auto it = arrayShapes.find(name);
        if (it != arrayShapes.end()) {
            return &it->second;
        }
        if (parent) {
            return parent->lookupArrayShapes(name);
        }
        return nullptr;
    }
  // Summary: Resolves symbol through current and parent scopes.
  // Input: symbol name.
  // Output: pointer to Symbol if found, otherwise nullptr.
  Symbol* lookup(const std::string& name) {
      auto it = symbolTable.find(name);
      if (it != symbolTable.end())
          return &it->second;
      else if (parent)
          return parent->lookup(name);
      return nullptr;
  }

  // Summary: Creates a child scope seeded from current scope snapshot.
  // Input: none.
  // Output: shared pointer to child SymbolTable.
  std::shared_ptr<SymbolTable> createChild() {
	return std::make_shared<SymbolTable>(std::make_shared<SymbolTable>(*this));
  }
};
