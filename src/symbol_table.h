#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

typedef enum { CONSTANT, 
               VARIABLE, 
               VOID_FUNC, 
               INT_FUNC,
               CONST_ARRAY,
               VAR_ARRAY, 
} type_t;

struct Symbol {
    type_t type;
    int val;
    Symbol() {}
    Symbol(type_t _type, int _val) : type(_type), val(_val) {}
};

// 出现的作用域的编号
static int global_scope_id = 0;


class SymbolTable {
    std::unordered_map<std::string, std::shared_ptr<Symbol>> table;
    std::shared_ptr<SymbolTable> prev;
    int scope_id;
public:
    SymbolTable(std::shared_ptr<SymbolTable> _prev = nullptr, int id = 0): 
                                                        prev(_prev), scope_id(id) {} 

    int insert(const std::string& ident, type_t type, int val) {
        auto it = table.find(ident);

        if(it != table.end()) {
            std::cerr << "Error: Symbol '" << ident << "' already declared.\n";
            return -1;
        }

        table[ident] = std::make_shared<Symbol>(type, val);
        return 0;
    }

    bool exist(const std::string& ident) {
        if(table.find(ident) != table.end()) {
            return true;
        }
        if(prev != nullptr) {
            return prev->exist(ident);
        }
        return false;
    }

    std::shared_ptr<Symbol> query(const std::string& ident) {
        auto it = table.find(ident);
        if(it != table.end()) {
            return it->second;
        }
        if(prev != nullptr) {
            return prev->query(ident);
        }
        return nullptr;
    }
    
    int query_scope(const std::string& ident) {
        auto it = table.find(ident);
        if(it != table.end()) {
            return scope_id;
        }
        if(prev != nullptr) {
            return prev->query_scope(ident);
        }
        return -1;
    }

    std::shared_ptr<SymbolTable> get_prev() {
        return prev;
    }

    int get_id() {
        return scope_id;
    }

    void clear() {
        table.clear();
    }

    void print() {
        std::cout << "IDENT\tTYPE\tVALUE\n";
        for(const auto& [ident, symbol] : table) {
            std::cout << ident << "\t";
            std::cout << ((symbol->type == CONSTANT) ? "CONSTANT" : "VARIABLE") << "\t";
            std::cout << symbol->val << std::endl;
        }
        std::cout <<std::endl;
    }
};

class SymbolTableList {
    std::shared_ptr<SymbolTable> current_table;
public:
    SymbolTableList() {current_table = std::make_shared<SymbolTable>();}

    int insert(const std::string& ident, type_t type, int val=0) {
        return current_table->insert(ident, type, val);
    }

    bool exist(const std::string& ident) {
        return current_table->exist(ident);
    }

    std::shared_ptr<Symbol> query(const std::string& ident) {
        return current_table->query(ident);
    }    

    int query_scope(const std::string& ident) {
        return current_table->query_scope(ident);
    }

    int current_scope_id() {
        return current_table->get_id();
    }

    void enter_scope() {
        ++global_scope_id;
        current_table = std::make_shared<SymbolTable>(current_table, global_scope_id);
    }

    void exit_scope() {
        auto prev = current_table->get_prev();
        if(prev != nullptr) {
            current_table.reset();
            current_table = prev;
        }
    }

    void init() {
        global_scope_id = 0;
    }
};