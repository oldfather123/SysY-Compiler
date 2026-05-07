#ifndef SYMBOL_H
#define SYMBOL_H
#include <string>
#include <vector>
#include <unordered_map>
#include"type.h"

class Symbol {
public:
    std::string getName() const; 
    explicit Symbol(const std::string& name);
    explicit Symbol() {};

private:
    std::string name;
};

Symbol *getSymbol(const std::string& name);
static std::unordered_map<std::string, Symbol*> symbolMap; 

class SymbolTable {
    public:
        //SymbolTable(){beginScope();}
        void enter(Symbol* sym, NodeAttribute value); 
        //void* look(Symbol* sym); 
        NodeAttribute look(Symbol* sym); 
        void beginScope(); 
        void endScope(); 
        int findScope(Symbol* sym);  
        //size_t tableSize() const;
        size_t scopesSize() const;
    private:
        //std::unordered_map<Symbol*, NodeAttribute> table;//疑点，如果多个作用域下都存储有某个变量，那么table中怎么存储？
        std::vector<std::unordered_map<Symbol*, NodeAttribute>> scopes;  
    };

class IdTable {
    private:
        std::unordered_map<std::string, Symbol*> id_table{};
    
    public:
        Symbol* add_id(std::string s);
};

class SymbolRegTable {
private:
    int current_scope = -1;
    std::vector<std::unordered_map<Symbol*, int>> symbol_table;

public:
    void enter(Symbol* C, int reg);
	int get_current_scope() { return current_scope; }

    // 返回离当前作用域最近的局部变量的alloca结果对应的寄存器编号(C为局部变量名对应的Symbol)
    int look(Symbol* C);
    void beginScope();
    void endScope();
	void display();
};

class SymbolDimTable {
private:
    int current_scope = -1;
    std::vector<std::unordered_map<Symbol*, std::vector<int>>> symbol_table;

public:
    void enter(Symbol* C, std::vector<int> dim);
	int get_current_scope() { return current_scope; }

    // 返回离当前作用域最近的局部变量的alloca结果对应的寄存器编号(C为局部变量名对应的Symbol)
    std::vector<int> look(Symbol* C);
    void beginScope();
    void endScope();
	void display();
};

#endif