#include "../include/symbol.h"

/* symbol */
Symbol *getSymbol(const std::string& name) {
    auto it = symbolMap.find(name);
    if (it != symbolMap.end()) {
        return it->second;  
    }

    Symbol* sym = new Symbol(name);
    symbolMap[name] = sym;
    return sym;
}

Symbol::Symbol(const std::string& name) : name(name) {}

std::string Symbol::getName() const {
    return name;
}


/* symbol table */

void SymbolTable::enter(Symbol* sym, NodeAttribute value) {
    //std::cout<<"ADD symbol "<<sym->getName()<<" in scope"<<scopes.size()-1<<std::endl;
    //std::cout<<"ADD symbol "<<sym->getName()<<" ,val.isptr="<<value.type->isPointer<<std::endl;
    if (!scopes.empty()&&sym!=nullptr) {
        int cur_scope=scopes.size();
        scopes[cur_scope-1][sym]=value;
    }
}

NodeAttribute SymbolTable::look(Symbol* sym) {//获取当前作用域下，该symbol对应的node(比如name,type)
    int cur_scope=scopes.size();
    // for(int i=0;i<cur_scope;++i)
    // {
    //     std::cout<<"第"<<i<<"层 symbol_table: ";
    //     for(auto [s,n]:scopes[i])
    //     {
    //         std::cout<<s->getName()<<" "<<n.type->isPointer<<",";
    //     }
    //     std::cout<<std::endl;
    // }
    for(int i=cur_scope-1;i>=0;--i)
    {
        auto it=scopes[i].find(sym);
        //return (it != scopes[cur_scope-1].end()) ? it->second : NodeAttribute();  
        if(it != scopes[i].end())
        {
            //std::cout<<"Find symbol "<<sym->getName()<<",the int value="<<scopes[i][sym].val.IntVal<<std::endl;
            // std::cout<<"Find symbol "<<sym->getName()<<",ptr="<<it->second.type->isPointer<<std::endl;
            return it->second;
        }
    }
  
    //std::cout<<"Not Find symbol "<<sym->getName()<<std::endl;
    return NodeAttribute();  
}

void SymbolTable::beginScope() {//scopes的size增大1
    scopes.push_back({}); 
}

void SymbolTable::endScope() {//scopes的size减少1
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

int SymbolTable::findScope(Symbol* sym) {
    int tabSize=scopes.size();
    //std::cout<<"scopeSize="<<scopes.size()<<std::endl;
    //std::cout<<"sym ="<<(sym->getName())<<std::endl;
    for (int i = tabSize - 1; i >= 0; --i) {
        auto it=scopes[i].find(sym);
        if(it!=scopes[i].end())
        {
            return i;
        }
    }
    return -1; 
}

// size_t SymbolTable::tableSize() const {
//     return table.size();
// }
size_t SymbolTable::scopesSize() const
{
    return scopes.size();
}

/* IdTable */
Symbol* IdTable::add_id(std::string s) {
    auto it = id_table.find(s);
    if (it == id_table.end()) {
        Symbol* new_symbol = new Symbol(s);
        id_table[s] = new_symbol;
        return new_symbol;
    } else {
        return id_table[s];
    }
}

/* SymbolRegTable */
void SymbolRegTable::enter(Symbol* C, int reg){
	symbol_table[current_scope][C] = reg; 
    return;
}
int SymbolRegTable::look(Symbol* C){
    for (int i = current_scope; i >= 0; --i) {
        auto it = symbol_table[i].find(C);
        if (it != symbol_table[i].end()) {
            return it->second;
        }
    }
    return -1;
}
void SymbolRegTable::beginScope(){
    ++current_scope;
    symbol_table.push_back(std::unordered_map<Symbol*, int>());
}
void SymbolRegTable::endScope(){
    --current_scope;
    symbol_table.pop_back();
}
void SymbolRegTable::display() {
	std::cerr << "SymbolRegTable::display()\n";
    for (int i = current_scope; i >= 0; --i) {
		std::cerr << "current_scope = " << i << " : ";
		for(auto it :symbol_table[i]){
			std::cerr << "<" <<it.first->getName() + "," << it.second << "> ";
		}
		std::cerr << "\n";
    }
}


/* SymbolDimTable */
std::vector<int> SymbolDimTable::look(Symbol* C) {
    for (int i = current_scope; i >= 0; --i) {
        auto it = symbol_table[i].find(C);
        if (it != symbol_table[i].end()) {
            return it->second;
        }
    }
    return std::vector<int>({-1}); // not found
}
void SymbolDimTable::enter(Symbol* C, std::vector<int> dim) { 
	symbol_table[current_scope][C] = dim; 
}

void SymbolDimTable::beginScope() {
    ++current_scope;
    symbol_table.push_back(std::unordered_map<Symbol*, std::vector<int>>());
}

void SymbolDimTable::endScope() {
    --current_scope;
    symbol_table.pop_back();
}

void SymbolDimTable::display() {
	std::cerr << "SymbolDimTable::display()\n";
    for (int i = current_scope; i >= 0; --i) {
		std::cerr << "current_scope = " << i << " : ";
		for(auto it :symbol_table[i]){
			std::cerr << "<" <<it.first->getName();
			for(int d : it.second) {
				std::cerr << d << " ";
			}
			std::cerr << ">";
		}
		std::cerr << "\n";
    }
}