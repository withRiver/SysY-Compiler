#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

typedef enum { CONSTANT, VARIABLE } type_t;

struct Symbol {
    type_t type;
    int val;
    Symbol() {}
    Symbol(type_t _type, int _val) : type(_type), val(_val) {}
};


class SymbolTable {
    std::unordered_map<std::string, std::shared_ptr<Symbol>> table;
public:
    int insert(const std::string& ident, type_t type, int val) {
        auto it = table.find(ident);

        if(it != table.end()) {
            std::cerr << "Error: Symbol '" << ident << "' already declared.\n";
            return -1;
        }

        table[ident] = std::make_shared<Symbol>(type, val);
        return 0;
    }

    int exist(const std::string& ident) {
        return table.find(ident) != table.end();
    }

    std::shared_ptr<Symbol> query(const std::string& ident) {
        auto it = table.find(ident);
        if(it != table.end()) {
            return it->second;
        }
        return nullptr;
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