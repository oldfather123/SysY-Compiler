
class SymbolTable{
private:
  enum Kind
  {
    kModule,
    kFunction,
    kBlock,
  };

  std::forward_list<std::pair<Kind, std::unordered_map<std::string, Value*>>> Scopes;

public:
  struct ModuleScope {
    SymbolTable& tables_ref;
    ModuleScope(SymbolTable& tables) : tables_ref(tables) {
      tables.enter(kModule);
    }
    ~ModuleScope() { tables_ref.exit(); }
  };
  struct FunctionScope {
    SymbolTable& tables_ref;
    FunctionScope(SymbolTable& tables) : tables_ref(tables) {
      tables.enter(kFunction);
    }
    ~FunctionScope() { tables_ref.exit(); }
  };
  struct BlockScope {
    SymbolTable& tables_ref;
    BlockScope(SymbolTable& tables) : tables_ref(tables) {
      tables.enter(kBlock);
    }
    ~BlockScope() { tables_ref.exit(); }
  };

  SymbolTable() = default;

  bool isModuleScope() const { return Scopes.front().first == kModule; }
  bool isFunctionScope() const { return Scopes.front().first == kFunction; }
  bool isBlockScope() const { return Scopes.front().first == kBlock; }
  Value *lookup(const std::string &name) const {
    for (auto &scope : Scopes) {
      auto iter = scope.second.find(name);
      if (iter != scope.second.end())
        return iter->second;
    }
    return nullptr;
  }
  auto insert(const std::string &name, Value *value) {
    assert(not Scopes.empty());
    return Scopes.front().second.emplace(name, value);
  }
private:
  void enter(Kind kind) {
    Scopes.emplace_front();
    Scopes.front().first = kind;
  }
  void exit() {
    Scopes.pop_front(); 
  }
  
};