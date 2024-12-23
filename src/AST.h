#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>
#include <cassert>
#include "symbol_table.h"

class BaseAST;
class CompUnitAST;
class CompUnitItemAST;

class DeclAST;
class ConstDeclAST;
class ConstDefAST;
class ConstInitValAST;
class VarDeclAST;
class VarDefAST;
class InitValAST;

class FuncDefAST;
//class FuncTypeAST;
class FuncFParamAST;
class BlockAST;
class BlockItemAST;
class StmtAST;

class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class FuncExpAST;
class MulExpAST;
class AddExpAST;
class RelExpAST;
class EqExpAST;
class LAndExpAST;
class LOrExpAST;
class ConstExpAST;

/*
CompUnit      ::= FuncDef;

Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT "=" ConstInitVal;
ConstInitVal  ::= ConstExp;
VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDef        ::= IDENT | IDENT "=" InitVal;
InitVal       ::= Exp;

FuncDef       ::= FuncType IDENT "(" ")" Block;
FuncType      ::= "int";

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;
Stmt          ::= LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "return" [Exp] ";";

Exp           ::= LOrExp;
LVal          ::= IDENT;
PrimaryExp    ::= "(" Exp ")" | LVal | Number;
Number        ::= INT_CONST;
UnaryExp      ::= PrimaryExp | UnaryOp UnaryExp;
UnaryOp       ::= "+" | "-" | "!";
MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp       ::= EqExp | LAndExp "&&" EqExp;
LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
ConstExp      ::= Exp;


*/

//使用到的寄存器编号
static int global_reg = 0;
//op对应到其IR表示
static std::unordered_map<std::string, std::string> op2IR = {
    {"+", "add"}, 
    {"-", "sub"},
    {"*", "mul"},
    {"/", "div"},
    {"%", "mod"},
    {"<", "lt"},
    {">", "gt"},
    {"<=", "le"},
    {">=", "ge"},
    {"==", "eq"},
    {"!=", "ne"},
};
//符号表
static SymbolTableList symbol_table;
//entry编号
static int entryNo = 0;
//if语句的数量，用于给if语句产生的基本块取名
static int global_if = 0;
//while语句的数量，用于给while语句产生的基本块取名
static int global_whileCnt = 0;
//当前while语句
static int global_curWhile = -1;
//while的嵌套关系，记录父节点
static std::unordered_map<int, int> while_fa = {{-1,-1}};
//记录是否在全局作用域中，针对全局变量声明
static int is_global = 1;

static void handle_aggregate(std::vector<std::pair<char, int>>&, std::vector<int>&, std::vector<int>&, 
                                int, int, const std::string&, char);

class BaseAST {
 public:
    virtual ~BaseAST() = default;    
    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
    virtual int eval() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> compunit_items;

    void Dump() const override {}

    std::string DumpIR() const override {
        // 将reg置0
        global_reg = 0; entryNo = 0;
        // 初始化symbol_table
        symbol_table = SymbolTableList();
        symbol_table.init();
        // 置为0
        global_if = 0;
        global_whileCnt = 0;
        global_curWhile = -1;

        // 在符号表中加入库函数
        symbol_table.insert("getint", INT_FUNC);
        symbol_table.insert("getch", INT_FUNC);
        symbol_table.insert("getarray", INT_FUNC);
        symbol_table.insert("putint", VOID_FUNC);
        symbol_table.insert("putch", VOID_FUNC);
        symbol_table.insert("putarray", VOID_FUNC);
        symbol_table.insert("starttime", VOID_FUNC);
        symbol_table.insert("stoptime", VOID_FUNC);

        // 在Koopa IR 中声明库函数
        std::cout << "decl @getint(): i32" << std::endl;
        std::cout << "decl @getch(): i32" << std::endl;        
        std::cout << "decl @getarray(*i32): i32" << std::endl;
        std::cout << "decl @putint(i32)" << std::endl; 
        std::cout << "decl @putch(i32)" << std::endl;
        std::cout << "decl @putarray(i32, *i32)" << std::endl;        
        std::cout << "decl @starttime()" << std::endl;
        std::cout << "decl @stoptime()" << std::endl; 
        std::cout << std::endl;

        for(auto& compunit_item : *compunit_items) {
            is_global = 1;
            compunit_item->DumpIR();
            std::cout << std::endl;
        }

        return "";
    }

    int eval() const override {return 0;}
};

class CompUnitItemAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> funcdef_decl;

    void Dump() const override {}

    std::string DumpIR() const override {
        return funcdef_decl->DumpIR();
    }

    int eval() const override {return 0;}
};

/*
class FuncTypeAST : public BaseAST {
 public:
    std::string type;

    void Dump() const override {}

    std::string DumpIR() const override {
        if(type == "int") {
            std::cout << ": i32";
        } 
        return "";
    }

    int eval() const override {return 0;}
};
*/

class BTypeAST : public BaseAST {
 public:
    std::string type;

    void Dump() const override {}

    std::string DumpIR() const override {
        if(type == "int") {
            std::cout << ": i32";
        } 
        return "";
    }

    int eval() const override {return 0;}
};

class FuncFParamAST : public BaseAST {
 public:
    enum TAG {INTEGER, ARRAY};
    TAG tag;
    std::unique_ptr<BaseAST> btype;
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> dim_list;
    void Dump() const override {}

    std::string DumpIR() const override {
        if(tag == INTEGER) {
            std::cout << "@" << ident << ": i32";
        } else if(tag == ARRAY) {
            std::cout << "@" << ident << ": *";
            for(int _ = 0; _ < dim_list->size(); ++_) {
                std::cout << "[";
            }
            std::cout << "i32";
            for(int i = dim_list->size() - 1; i >= 0; --i) {
                std::cout << ", " << dim_list->at(i)->eval() << "]";
            }
        }
        return "";
    }

    int eval() const override {return 0;}
};

class FuncDefAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_fparams;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {}

    std::string DumpIR() const override {
        is_global = 0;
        global_reg = 0;

        std::string type = dynamic_cast<BTypeAST*>(func_type.get())->type;

        symbol_table.insert(ident, type == "int" ? INT_FUNC : VOID_FUNC);

        symbol_table.enter_scope();
        std::cout << "fun @" << ident << "(";
        int n = func_fparams->size();
        int i  = 0;
        for(auto& func_fparam : *func_fparams) {
            func_fparam->DumpIR();
            if(i != n - 1) {std::cout << ", ";}
            ++i;
        }
        std::cout << ") "; 
        func_type->DumpIR();
        std::cout << " {\n";
        std::cout << "%LHR_entry_" << ident << ":\n";
        for(auto& func_fparam: *func_fparams) {
            auto param = dynamic_cast<FuncFParamAST*>(func_fparam.get());
            std::string param_name = param->ident;
            if(param->tag == FuncFParamAST::INTEGER) {
                symbol_table.insert(param_name, VARIABLE);
                std::cout << "  @" << param_name << "_" << symbol_table.current_scope_id();
                std::cout << " = alloc i32" << std::endl;
                std::cout << "  store @" << param_name;
                std::cout << ", @" << param_name << "_" << symbol_table.current_scope_id();
                std::cout << std::endl;
            } else if(param->tag == FuncFParamAST::ARRAY) {
                symbol_table.insert(param_name, POINTER, param->dim_list->size() + 1);
                std::cout << "  @" << param_name << "_" << symbol_table.current_scope_id();
                std::cout << " = alloc *";
                for(int _ = 0; _ < param->dim_list->size(); ++_) {
                    std::cout << "[";
                }
                std::cout << "i32";
                for(int i = param->dim_list->size() - 1; i >= 0; --i) {
                    std::cout << ", " << param->dim_list->at(i)->eval() << "]";
                }           
                std::cout << std::endl;            
                std::cout << "  store @" << param_name << ", @" << param_name << "_" << symbol_table.current_scope_id();
                std::cout << std::endl;         
            }
        }

        if(block->DumpIR() != "RETURN") {
            if(type == "int") {
                std::cout << "  ret 0" << std::endl;
            } else {
                std::cout << "  ret" << std::endl;
            }
        };
        std::cout << "}";
        std::cout << std::endl;

        symbol_table.exit_scope();
        return "";
    }

    int eval() const override {return 0;}
};


// Block ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {
 public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> blockitem_vec;   

    void Dump() const override {}

    std::string DumpIR() const override {
        // 进入一个新的作用域
        symbol_table.enter_scope();
        std::string str;
        for(auto& blockitem : *blockitem_vec) {
            str = blockitem->DumpIR();
            if(str == "RETURN") {
                break;
            }
        }
        // 退出该作用域
        symbol_table.exit_scope();
        return str;
    }

    int eval() const override {return 0;} 
};

// BlockItem ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
 public: 
    enum TAG {DECL, STMT};
    TAG tag;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {}
    std::string DumpIR() const override {
        switch(tag) {
            case DECL: decl->DumpIR(); break;
            case STMT: return stmt->DumpIR(); break;
            default: break;
        }
        return "";
    }

    int eval() const override {return 0;}
};


// Decl ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> const_var_decl;
    void Dump() const override {}
    std::string DumpIR() const override {
        const_var_decl->DumpIR();
        return "";
    }
    int eval() const override {return 0;}
};

// ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> constdef_vec;
    void Dump() const override {}
    std::string DumpIR() const override {
        for(auto& constdef : *constdef_vec) {
            constdef->DumpIR();
        }
        return "";
    }
    int eval() const override {return 0;}
};

// ConstInitVal ::= ConstExp | '{ ConstInitValList '}' ;
class ConstInitValAST : public BaseAST {
 public:
    enum TAG {CONSTEXP, CONSTINITVAL};
    TAG tag;
    std::unique_ptr<BaseAST> const_exp;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> constinitval_list;
    void Dump() const override {}
    std::string DumpIR() const override {
        return const_exp->DumpIR();
    }
    int eval() const override {
        assert(tag == CONSTEXP);
        return const_exp->eval();
    }
    std::vector<std::pair<char,int>> get_aggregate(std::vector<int>::iterator s, std::vector<int>::iterator e) const {
        std::vector<std::pair<char, int>> aggregate;
        for(auto& constinitval: *constinitval_list) {
            auto cit = dynamic_cast<ConstInitValAST*>(constinitval.get());
            if(cit->tag == CONSTEXP) {
                aggregate.push_back(std::make_pair(0, cit->eval()));
            } else {
                auto it = s;
                ++it;
                for(; it != e; ++it) {
                    if(aggregate.size() % (*it) == 0) {
                        auto sub = cit-> get_aggregate(it, e);
                        aggregate.insert(aggregate.end(), sub.begin(), sub.end());
                        break;
                    }
                }
            }
        }
        // 补0
        aggregate.insert(aggregate.end(), (*s) - aggregate.size(), std::make_pair(0,0));
        return aggregate;
    }
};

// ConstDef ::= IDENT DimList "=" ConstInitVal;
class ConstDefAST : public BaseAST {
 public:
    std::string ident;
    std::unique_ptr<BaseAST> const_initval;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> dim_list;
    void Dump() const override {}
    std::string DumpIR() const override {
        if(dim_list->empty()) {
            symbol_table.insert(ident, CONSTANT, const_initval->eval());
        } else {
            symbol_table.insert(ident, CONST_ARRAY, dim_list->size());
            if(is_global) {
                std::cout << "global ";
            } else {
                std::cout << "  ";
            }
            int len = dim_list->size();
            int scope_id = symbol_table.current_scope_id();
            std::string name = ident + "_" + std::to_string(scope_id);
            std::cout << "@" << name << " = " << "alloc ";
            for(int _ = 0; _ < len; ++_) {
                std::cout << "[";
            }
            std::cout<< "i32";
            auto words = std::vector<int>();
            auto lens = std::vector<int>();
            for(int i = len - 1; i >= 0; --i) {
                int val = dim_list->at(i)->eval();
                lens.push_back(val);
                if(words.empty()) {
                    words.push_back(val);
                } else {
                    words.push_back(val * words.back());
                }
                std::cout << ", " << val << "]";
            }
            std::reverse(words.begin(), words.end());
            std::reverse(lens.begin(), lens.end());
            std::vector<std::pair<char, int>> aggregate = dynamic_cast<ConstInitValAST*>(const_initval.get())
                                                        ->get_aggregate(words.begin(), words.end());
            if(is_global) {
                std::cout << ", ";
                handle_aggregate(aggregate, lens, words, 0, 0, ident, 'G');
                std::cout << std::endl;
            } else {
                std::cout << std::endl;
                handle_aggregate(aggregate, lens, words, 0, 0, ident, 'C');
            }
        }
        return "";
    }
    int eval() const override {return 0;}
};



// VarDecl ::= BType VarDef {"," VarDef} ";";
class VarDeclAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> vardef_vec; 
    void Dump() const override {}
    std::string DumpIR() const override {
        for(auto& vardef : *vardef_vec) {
            vardef->DumpIR();
        }
        return "";
    }
    int eval() const override {return 0;}
};

// InitVal ::= Exp | '{' InitValList '}' ;
class InitValAST : public BaseAST {
 public:
    enum TAG {EXP, INITVAL};
    TAG tag;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> initval_list;
    void Dump() const override {}
    std::string DumpIR() const override {
        return exp->DumpIR();
    }
    int eval() const override {
        return exp->eval();
    }
    std::vector<std::pair<char, int>> get_aggregate(std::vector<int>::iterator s, std::vector<int>::iterator e) const {
        if(is_global) {
            std::vector<std::pair<char, int>> aggregate;
            for(auto& initval: *initval_list) {
                auto cit = dynamic_cast<InitValAST*>(initval.get());
                if(cit->tag == EXP) {
                    aggregate.push_back(std::make_pair(0, cit->eval()));
                } else {
                    auto it = s;
                    ++it;
                    for(; it != e; ++it) {
                        if(aggregate.size() % (*it) == 0) {
                            auto sub = cit-> get_aggregate(it, e);
                            aggregate.insert(aggregate.end(), sub.begin(), sub.end());
                            break;
                        }
                    }
                }
            }
            aggregate.insert(aggregate.end(), (*s) - aggregate.size(), std::make_pair(0, 0));
            return aggregate;            
        } else {
            std::vector<std::pair<char, int>> aggregate;
            for(auto& initval: *initval_list) {
                auto iv = dynamic_cast<InitValAST*>(initval.get());
                if(iv->tag == EXP) {
                    std::string num = iv->DumpIR();
                    //std::cout << std::endl << num << std::endl;
                    if(!num.empty()) {
                        aggregate.push_back(std::make_pair(0, stoi(num)));
                    } else {
                        aggregate.push_back(std::make_pair(1, global_reg - 1));
                    }
                } else {
                    auto it = s;
                    ++it;
                    for(; it != e; ++it) {
                        if(aggregate.size() % (*it) == 0) {
                            auto sub = iv-> get_aggregate(it, e);
                            aggregate.insert(aggregate.end(), sub.begin(), sub.end());
                            break;
                        }
                    }                    
                }
            }
            aggregate.insert(aggregate.end(), (*s) - aggregate.size(), std::make_pair(0, 0));
            //for(auto& v: aggregate) {std::cout << " " << v.second << " ";}
            return aggregate;
        }
    }
};

// VarDef ::= IDENT DimList | IDENT DimList "=" InitVal;
class VarDefAST : public BaseAST {
 public:
    enum TAG {IDENT, IDENT_EQ_VAL};
    TAG tag;
    std::string ident;
    std::unique_ptr<BaseAST> initval;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> dim_list;
    void Dump() const override {}
    std::string DumpIR() const override {
        // (global )@x = alloc i32(, zeroinit)
        if(dim_list->empty()) {
            if(is_global) {std::cout << "global";}
            int scope_id = symbol_table.current_scope_id();
            std::string name = ident + "_" + std::to_string(scope_id);
            std::cout << "  @" << name << " = alloc i32";
            if(is_global && tag == IDENT) {std::cout << ", zeroinit";}
            if(is_global && tag == IDENT_EQ_VAL) {std::cout << ", " << initval->eval();}
            std::cout << std::endl;
            // store 10, @x
            if(is_global == 0 && tag == IDENT_EQ_VAL) {
                std::string num = initval->DumpIR();
                if(!num.empty()) {
                    std::cout << "  store " << num << ", @" << name;
                } else {
                    std::cout << "  store %" << global_reg - 1 << ", @" << name;
                }
                std::cout << std::endl;
            }
            symbol_table.insert(ident, VARIABLE);
        } else {
            symbol_table.insert(ident, VAR_ARRAY, dim_list->size());
            if(is_global) {
                std::cout << "global";
            } else {
                std::cout << "  ";
            }
            int len = dim_list->size();
            int scope_id = symbol_table.current_scope_id();
            std::string name = ident + "_" + std::to_string(scope_id);
            std::cout << "@" << name << " = " << "alloc ";
            for(int _ = 0; _ < len; ++_) {
                std::cout << "[";
            }
            std::cout<< "i32";
            auto words = std::vector<int>();
            auto lens = std::vector<int>();
            for(int i = len - 1; i >= 0; --i) {
                int val = dim_list->at(i)->eval();
                lens.push_back(val);
                if(words.empty()) {
                    words.push_back(val);
                } else {
                    words.push_back(val * words.back());
                }
                std::cout << ", " << val << "]";
            }
            std::reverse(words.begin(), words.end());
            std::reverse(lens.begin(), lens.end());
            if(is_global) {
                if(tag == IDENT) {
                    std::cout << ", zeroinit" << std::endl;
                } else {
                    std::vector<std::pair<char, int>> aggregate = dynamic_cast<InitValAST*>(initval.get())
                                                                ->get_aggregate(words.begin(), words.end());
                    std::cout << ", ";
                    handle_aggregate(aggregate, lens, words, 0, 0, ident, 'G');
                    std::cout << std::endl;
                }
            } else {
                if(tag == IDENT) {
                    std::cout << std::endl;
                } else {
                    std::vector<std::pair<char, int>> aggregate = dynamic_cast<InitValAST*>(initval.get())
                                                                ->get_aggregate(words.begin(), words.end());
                    std::cout << std::endl;
                    //for(auto& v: aggregate) {std::cout << " " << v.second << " ";}
                    handle_aggregate(aggregate, lens, words, 0, 0, ident, 'V');                    
                }
            }

        }
        return "";
    } 

    int eval() const override {return 0;}
};



// LVal ::= IDENT IndexList
class LValAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> index_list;
    void Dump() const override {}
    std::string DumpIR() const override {
        assert(symbol_table.exist(ident));
        auto symb = symbol_table.query(ident);
        int scope = symbol_table.query_scope(ident);
        if(symb->type == CONSTANT) {
            return std::to_string(symb->val);
        }
        else if(symb->type == VARIABLE) {
            // load @x
            std::cout << "  %" << global_reg << " = load @" << ident << "_" << scope;
            std::cout << std::endl;
            ++global_reg; 
        } else if(symb->type == CONST_ARRAY || symb->type == VAR_ARRAY) {
            if(symb->val == index_list->size()) {
                for(int i = 0; i < index_list->size(); ++i) {
                    int last_ptr = global_reg - 1;
                    auto& index = index_list->at(i);
                    std::string num = index->DumpIR();
                    std::cout << "  %" << global_reg << " = getelemptr ";
                    if(i == 0) {
                        std::cout << "@" << ident << "_" << scope << ", ";
                    } else {
                        std::cout << "%" << last_ptr << ", ";
                    }
                    if(!num.empty()) {
                        std::cout << num << std::endl;
                    } else {
                        std::cout << "%" << global_reg - 1 << std::endl;
                    }
                    ++global_reg;
                }
                std::cout << "  %" << global_reg << " = load %" << global_reg - 1 << std::endl;
                ++global_reg;
            } else {
                for(int i = 0; i < index_list->size(); ++i) {
                    int last_ptr = global_reg - 1;
                    auto& index = index_list->at(i);
                    std::string num = index->DumpIR();
                    std::cout << "  %" << global_reg << " = getelemptr ";
                    if(i == 0) {
                        std::cout << "@" << ident << "_" << scope << ", ";
                    } else {
                        std::cout << "%" << last_ptr << ", ";
                    }
                    if(!num.empty()) {
                        std::cout << num << std::endl;
                    } else {
                        std::cout << "%" << global_reg - 1 << std::endl;
                    }
                    ++global_reg;
                }
                std::cout << "  %" << global_reg << " = getelemptr ";
                if(index_list->size() == 0) {
                    std::cout << "@" << ident << "_" << scope;
                } else {
                    std::cout << "%" << global_reg - 1;
                }
                std::cout << ", 0" << std::endl;
                ++global_reg;
            }
        } else if(symb->type == POINTER) {
            if(symb->val == index_list->size()) {
                std::cout << "  %" << global_reg << " = load @" << ident << "_" << scope << std::endl;
                ++global_reg;
                for(int i = 0; i < index_list->size(); ++i) {
                    int last_ptr = global_reg - 1;
                    auto& index = index_list->at(i);
                    std::string num = index->DumpIR();
                    if(i == 0) {
                        std::cout << "  %" << global_reg << " = getptr ";
                        std::cout << "%" << last_ptr << ", ";
                    } else {
                        std::cout << "  %" << global_reg << " = getelemptr ";
                        std::cout << "%" << last_ptr << ", ";
                    }
                    if(!num.empty()) {
                        std::cout << num << std::endl;
                    } else {
                        std::cout << "%" << global_reg - 1 << std::endl;
                    }
                    ++global_reg;
                }
                std::cout << "  %" << global_reg << " = load %" << global_reg - 1 << std::endl;
                ++global_reg;     
            } else {
                std::cout << "  %" << global_reg << " = load @" << ident << "_" << scope << std::endl;
                ++global_reg;
                for(int i = 0; i < index_list->size(); ++i) {
                    int last_ptr = global_reg - 1;
                    auto& index = index_list->at(i);
                    std::string num = index->DumpIR();
                    if(i == 0) {
                        std::cout << "  %" << global_reg << " = getptr ";
                        std::cout << "%" << last_ptr << ", ";
                    } else {
                        std::cout << "  %" << global_reg << " = getelemptr ";
                        std::cout << "%" << last_ptr << ", ";
                    }
                    if(!num.empty()) {
                        std::cout << num << std::endl;
                    } else {
                        std::cout << "%" << global_reg - 1 << std::endl;
                    }
                    ++global_reg;
                }
                if(index_list->size() == 0) {
                    std::cout << "  %" << global_reg << " = getptr %";
                } else {
                    std::cout << "  %" << global_reg << " = getelemptr %";
                }
                std::cout << global_reg - 1 << ", 0" << std::endl;
                ++global_reg;
            }
        }
        return "";
    }

    int eval() const override {
        auto symb = symbol_table.query(ident);
        return symb->val;
    }
};


//Exp ::= LOrExp;
class ExpAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> lor_exp;

    void Dump() const override {
        std::cout << "ExpAST { ";
        lor_exp->Dump();
        std::cout << " }";
    }

    std::string DumpIR() const override {
        return lor_exp->DumpIR();
    }

    int eval() const override {
        return lor_exp->eval();
    }
};


// PrimaryExp ::= "(" Exp ")" | LVal | Number;
class PrimaryExpAST : public BaseAST {
 public:
    enum TAG {BRAKET_EXP, LVAL, NUMBER};
    TAG tag;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    int number;

    void Dump() const override {
        std::cout << "PrimaryExp { ";
        switch(tag) {
            case BRAKET_EXP:
                std::cout << "( ";
                exp->Dump();
                std::cout << " )";
                break;
            case LVAL:
                lval->Dump();
                break;
            case NUMBER:
                std::cout << number;
                break;
            default:
                break;
        }
        std::cout << " }";
    }

    std::string DumpIR() const override {
        switch(tag) {
            case BRAKET_EXP:
                return exp->DumpIR(); break;
            case LVAL:
                return lval->DumpIR(); break;
            case NUMBER:
                return std::to_string(number); break;
            default: break;
        }
        return "";
    }

    int eval() const override {
        switch(tag) {
            case BRAKET_EXP:
                return exp->eval(); break;
            case LVAL:
                return lval->eval(); break;
            case NUMBER:
                return number; break;
            default: break;
        }
        return 0;        
    }
};

//UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | FuncExp;
class UnaryExpAST : public BaseAST {
 public:
    enum TAG { PRIMARY_EXP, OP_UNARY_EXP, FUNC_EXP};
    TAG tag;
    std::unique_ptr<BaseAST> primary_exp;
    std::string unary_op;
    std::unique_ptr<BaseAST> unary_exp;
    std::unique_ptr<BaseAST> func_exp;

    void Dump() const override {
        std::cout << "UnaryExpAST { ";
        switch(tag) {
            case PRIMARY_EXP:
                primary_exp->Dump();
                break;
            case OP_UNARY_EXP:
                std::cout << unary_op << " ";
                unary_exp->Dump();
                break;
            default:
                break;
        }
        std::cout << " }";
    }

    std::string DumpIR() const override {
        std::string num;
        switch(tag) {
            case PRIMARY_EXP:
                return primary_exp->DumpIR();
                break;
            case OP_UNARY_EXP:
                num = unary_exp->DumpIR();
                if(unary_op == "+") {
                    if(!num.empty()) {
                        return num;
                    }
                    else {
                        ;
                    }
                }
                if(unary_op == "-") {
                    if(!num.empty()) {
                        std::cout << "  %" << global_reg << " = sub 0, " << num;
                    }
                    else {
                        std::cout << "  %" << global_reg << " = sub 0, %" << global_reg - 1;
                    }
                    std::cout << std::endl;
                    ++global_reg;
                }
                else if(unary_op == "!") {
                    if(!num.empty()) {
                        std::cout << "  %" << global_reg << " = eq 0, " << num; 
                    }
                    else {
                        std::cout <<"  %" << global_reg << " = eq 0, %" << global_reg - 1;
                    }
                    std::cout << std::endl;
                    ++global_reg;
                }
                break;
            case FUNC_EXP:
                func_exp->DumpIR();
                break;
            default:
                break;
        }
        return "";
    }

    int eval() const override {
        switch(tag) {
            case PRIMARY_EXP:
                return primary_exp->eval();
                break;
            case OP_UNARY_EXP:
                if(unary_op == "+") {
                    return unary_exp->eval();
                } else if(unary_op == "-") {
                    return -unary_exp->eval();
                } else if(unary_op == "!") {
                    return !unary_exp->eval();
                }
                break;
            default: break;
        }
        return 0;
    }
};

// FuncExp ::= IDENT '(' FuncRParams ')'
class FuncExpAST : public BaseAST {
 public:
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_rparams;

    void Dump() const override {}
    std::string DumpIR() const override {
        auto func = symbol_table.query(ident);
        assert(func != nullptr && (func->type == INT_FUNC || func->type == VOID_FUNC));

        // 调用函数前计算exp
        auto params = std::vector<std::pair<char, int>>();
        for(auto& func_rparam : *func_rparams) {
            std::string num = func_rparam->DumpIR();
            if(!num.empty()) {
                params.push_back(std::make_pair(0, std::stoi(num)));
            } else {
                params.push_back(std::make_pair(1, global_reg - 1));
            }
        }

        if(func->type == VOID_FUNC) {
            std::cout << "  call @" << ident << "(";
        } else if (func->type == INT_FUNC) {
            std::cout << "  %" << global_reg << " = call @" << ident << "(";
            ++global_reg;
        }

        int n = params.size();
        int i = 0;
        for(auto& param: params) {
            if(param.first == 1) {
                std::cout << "%";
            }
            std::cout << param.second;
            if(i != n - 1) {std::cout << ", ";}
            ++i;
        }

        std::cout << ")" << std::endl;
        return "";
    }
    int eval() const override {return 0;}
};

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
class MulExpAST : public BaseAST {
 public:
    enum Tag {UNARY_EXP, MULEXP_OP_UNARYEXP};
    Tag tag;
    std::unique_ptr<BaseAST> unary_exp;
    std::string mul_op;
    std::unique_ptr<BaseAST> mul_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case UNARY_EXP:
                return unary_exp->DumpIR();
                break;
            case MULEXP_OP_UNARYEXP:
                left_num = mul_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = unary_exp->DumpIR();
                right_reg = global_reg - 1;
                std::cout << "  %" << global_reg << " = " << op2IR[mul_op];
                if(!left_num.empty()) {
                    std::cout << " " << left_num << ",";
                } else {
                    std::cout << " %" << left_reg << ",";
                }
                if(!right_num.empty()) {
                    std::cout << " " << right_num;
                } else {
                    std::cout << " %" << right_reg;
                }
                std::cout << std::endl;
                ++global_reg;
                break;
            default: break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case UNARY_EXP:
                return unary_exp->eval();
                break;
            case MULEXP_OP_UNARYEXP:
                left = mul_exp->eval();
                right = unary_exp->eval();
                if(mul_op == "*") { return left * right;} 
                else if(mul_op == "/") { return left / right;}
                else if(mul_op == "%") { return left % right;}
                break;
            default: break;
        }
        return 0;
    }
};

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
class AddExpAST : public BaseAST {
 public:
    enum TAG {MUL_EXP, ADDEXP_OP_MULEXP};
    TAG tag;
    std::unique_ptr<BaseAST> mul_exp;
    std::string add_op;
    std::unique_ptr<BaseAST> add_exp;

    void Dump() const override {}
    
    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case MUL_EXP:
                return mul_exp->DumpIR(); break;
            case ADDEXP_OP_MULEXP:
                left_num = add_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = mul_exp->DumpIR();
                right_reg = global_reg - 1;
                std::cout << "  %" << global_reg << " = " << op2IR[add_op];
                if(!left_num.empty()) {
                    std::cout << " " << left_num << ",";
                } else {
                    std::cout << " %" << left_reg << ",";
                }
                if(!right_num.empty()) {
                    std::cout << " " << right_num;
                } else {
                    std::cout << " %" << right_reg;
                }
                std::cout << std::endl;
                ++global_reg;
                break;
            default: break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case MUL_EXP:
                return mul_exp->eval();
                break;
            case ADDEXP_OP_MULEXP:
                left = add_exp->eval();
                right = mul_exp->eval();
                if(add_op == "+") { return left + right;} 
                else if(add_op == "-") { return left - right;}
                break;
            default: break;
        }
        return 0;
    }
};

// RelExp ::= AddExp | RelExp RelOp AddExp;
// RelOp ::= "<" | ">" | "<=" | ">="
class RelExpAST : public BaseAST {
 public:
    enum TAG {ADD_EXP, RELEXP_OP_ADDEXP};
    TAG tag;
    std::unique_ptr<BaseAST> rel_exp;
    std::string rel_op;
    std::unique_ptr<BaseAST> add_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case ADD_EXP:
                return add_exp->DumpIR();
                break;
            case RELEXP_OP_ADDEXP:
                left_num = rel_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = add_exp->DumpIR();
                right_reg = global_reg - 1;
                std::cout << "  %" << global_reg << " = " << op2IR[rel_op];
                if(!left_num.empty()) {
                    std::cout << " " << left_num << ",";
                } else {
                    std::cout << " %" << left_reg << ",";
                }
                if(!right_num.empty()) {
                    std::cout << " " << right_num;
                } else {
                    std::cout << " %" << right_reg;
                }
                std::cout << std::endl;
                ++global_reg;
                break;
            default:
                break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case ADD_EXP:
                return add_exp->eval();
                break;
            case RELEXP_OP_ADDEXP:
                left = rel_exp->eval();
                right = add_exp->eval();
                if(rel_op == "<") { return left < right;} 
                else if(rel_op == ">") { return left > right;}
                else if(rel_op == "<=") { return left <= right;}
                else if(rel_op == ">=") { return left >= right;}
                break;
            default: break;
        }
        return 0;
    }
};

// EqExp ::= RelExp | EqExp EqOp RelExp;
// EqOp ::= "==" | "!="
class EqExpAST : public BaseAST {
public:
    enum TAG {REL_EXP, EQEXP_OP_RELEXP};
    TAG tag;
    std::unique_ptr<BaseAST> eq_exp;
    std::string eq_op;
    std::unique_ptr<BaseAST> rel_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case REL_EXP:
                return rel_exp->DumpIR();
                break;
            case EQEXP_OP_RELEXP:
                left_num = eq_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = rel_exp->DumpIR();
                right_reg = global_reg - 1;
                std::cout << "  %" << global_reg << " = " << op2IR[eq_op];
                if(!left_num.empty()) {
                    std::cout << " " << left_num << ",";
                } else {
                    std::cout << " %" << left_reg << ",";
                }
                if(!right_num.empty()) {
                    std::cout << " " << right_num;
                } else {
                    std::cout << " %" << right_reg;
                }
                std::cout << std::endl;
                ++global_reg;
                break;
            default:
                break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case REL_EXP:
                return rel_exp->eval();
                break;
            case EQEXP_OP_RELEXP:
                left = eq_exp->eval();
                right = rel_exp->eval();
                if(eq_op == "==") { return left == right;} 
                else if(eq_op == "!=") { return left != right;}
                break;
            default: break;
        }
        return 0;
    }
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp;
// 短路求值: int andRes = 0; if (LAndExp != 0) {andRes = EqExp != 0;}
class LAndExpAST : public BaseAST {
 public:
    enum TAG {EQ_EXP, LANDEXP_AND_EQEXP};
    TAG tag;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int cur_ifNo = global_if;
        std::string ident;
        switch(tag) {
            case EQ_EXP:
                return eq_exp->DumpIR();
                break;
            case LANDEXP_AND_EQEXP:
                ++global_if;
                ident = "@andRes_" + std::to_string(cur_ifNo);
                // @andRes_2 = alloc i32
                // store 0, @andRes_2 
                std::cout << "  @andRes_" << cur_ifNo << " = alloc i32" << std::endl;
                std::cout << "  store 0, @andRes_" << cur_ifNo << std::endl;
                symbol_table.insert(ident, VARIABLE, 0);
                left_num = land_exp->DumpIR();
                // %3 = ne %2, 0
                if(!left_num.empty()) {
                    std::cout << "  %" << global_reg << " = ne " << left_num << ", 0" << std::endl;
                } else {
                    std::cout << "  %" << global_reg << " = ne %" << global_reg - 1 << ", 0" << std::endl;
                }
                ++global_reg;
                // br %3, %then, %if_end
                std::cout << "  br " << "%" << global_reg - 1 << ", %then_" << cur_ifNo << ", %if_end_" << cur_ifNo << std::endl;
                std::cout << std::endl;
                // %then:
                std::cout << "%then_" << cur_ifNo << ":\n";
                right_num = eq_exp->DumpIR();
                if(!right_num.empty()) {
                    std::cout << "  %" << global_reg << " = ne " << right_num << ", 0" << std::endl;
                } else {
                    std::cout << "  %" << global_reg << " = ne %" << global_reg - 1 << ", 0" << std::endl;
                }
                ++global_reg;
                std::cout << "  store %" << global_reg - 1 << ", @andRes_" << cur_ifNo << std::endl;  
                std::cout << "  jump " << "%if_end_" << cur_ifNo << std::endl;
                std::cout << std::endl;
                // %if_end:
                std::cout << "%if_end_" << cur_ifNo << ":\n";
                std::cout << "  %" << global_reg << " = load @andRes_" << cur_ifNo << std::endl;
                ++global_reg;
                break;
            default:
                break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case EQ_EXP:
                return eq_exp->eval();
                break;
            case LANDEXP_AND_EQEXP:
                left = land_exp->eval();
                if(left == 0) {return 0;}
                right = eq_exp->eval();
                return right != 0;
                break;
            default: break;
        }
        return 0;
    }
};

// LOrExp  ::= LAndExp | LOrExp "||" LAndExp;
// 短路求值: int orRes = 1; if (LOrExp == 0) {orRes = LAndExp != 0;} 
class LOrExpAST : public BaseAST {
 public:
    enum TAG {LAND_EXP, LOREXP_OR_LANDEXP};
    TAG tag;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int cur_ifNo = global_if;
        std::string ident;
        switch(tag) {
            case LAND_EXP:
                return land_exp->DumpIR();
                break;
            case LOREXP_OR_LANDEXP:
                ++global_if;
                ident = "@orRes_" + std::to_string(cur_ifNo);
                // @orRes_2 = alloc i32
                // store 1, @orRes_2 
                std::cout << "  @orRes_" << cur_ifNo << " = alloc i32" << std::endl;
                std::cout << "  store 1, @orRes_" << cur_ifNo << std::endl;
                symbol_table.insert(ident, VARIABLE, 0);
                left_num = lor_exp->DumpIR();
                // %3 = eq %2, 0
                if(!left_num.empty()) {
                    std::cout << "  %" << global_reg << " = eq " << left_num << ", 0" << std::endl;
                } else {
                    std::cout << "  %" << global_reg << " = eq %" << global_reg - 1 << ", 0" << std::endl;
                }
                ++global_reg;
                // br %3, %then, %if_end
                std::cout << "  br " << "%" << global_reg - 1 << ", %then_" << cur_ifNo << ", %if_end_" << cur_ifNo << std::endl;
                std::cout << std::endl;
                // %then:
                std::cout << "%then_" << cur_ifNo << ":\n";
                right_num = land_exp->DumpIR();
                if(!right_num.empty()) {
                    std::cout << "  %" << global_reg << " = ne " << right_num << ", 0" << std::endl;
                } else {
                    std::cout << "  %" << global_reg << " = ne %" << global_reg - 1 << ", 0" << std::endl;
                }
                ++global_reg;
                std::cout << "  store %" << global_reg - 1 << ", @orRes_" << cur_ifNo << std::endl;  
                std::cout << "  jump " << "%if_end_" << cur_ifNo << std::endl;
                std::cout << std::endl;
                // %if_end:
                std::cout << "%if_end_" << cur_ifNo << ":\n";
                std::cout << "  %" << global_reg << " = load @orRes_" << cur_ifNo << std::endl;
                ++global_reg;
                break;
            default:
                break;
        }
        return "";
    }

    int eval() const override {
        int left, right;
        switch(tag) {
            case LAND_EXP:
                return land_exp->eval();
                break;
            case LOREXP_OR_LANDEXP:
                left = lor_exp->eval();
                if(left == 1) {return 1;}
                right = land_exp->eval();
                return right != 0;
                break;
            default: break;
        }
        return 0;
    }
};

// ConstExp ::= Exp;
class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override {}
    std::string DumpIR() const override {return exp->DumpIR();}
    int eval() const override {
        return exp->eval();
    } 
};

// Stmt ::= LVal "=" Exp ";" | [Exp] ";" | Block | "return" [Exp] ";" ;
//      | = "if" "(" Exp ")" Stmt ["else" Stmt]
class StmtAST : public BaseAST {
 public:
    enum TAG {ASSIGN, EMPTY, EXP, BLOCK, RETURN_EXP, RETURN_EMPTY, 
                IF, IFELSE, WHILE, BREAK, CONTINUE};
    TAG tag;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string num;
        int cur_ifNo = global_if;
        int old_while;
        int end_required = 0; // 判断if是否需要%end
        // std::string stmt_ret; // if_stmt和else_stmt的dumpIR()返回值
        LValAST* lval_ptr;
        int last_ptr;
        int exp_reg;
        std::string num2; 
        Symbol sym;
        switch(tag) {
            case RETURN_EXP:
                num = exp->DumpIR();
                if(!num.empty()) {
                    std::cout << "  ret " << num << std::endl;
                } else {
                    std::cout << "  ret %" << global_reg - 1 << std::endl;
                }
                return "RETURN";
                break;
            case ASSIGN:
                lval_ptr = dynamic_cast<LValAST*>(lval.get());
                num = exp->DumpIR();
                exp_reg = global_reg - 1;
                sym = *symbol_table.query(lval_ptr->ident);
                if(lval_ptr->index_list->empty()) {
                    if(!num.empty()) {
                        std::cout << "  store " << num << ", @";
                    } else {
                        std::cout << "  store %" << global_reg - 1 <<", @";
                    }
                    std::cout << lval_ptr->ident;
                    std::cout << "_" << symbol_table.query_scope(lval_ptr->ident);
                    std::cout << std::endl;
                } else if(sym.type == VAR_ARRAY){
                    for(int i = 0; i < lval_ptr->index_list->size(); ++i) {
                        last_ptr = global_reg - 1;
                        num2 = lval_ptr->index_list->at(i)->DumpIR();
                        std::cout << "  %" << global_reg << " = getelemptr ";
                        if(i == 0) {
                            std::cout << "@" << lval_ptr->ident << "_" <<  symbol_table.query_scope(lval_ptr->ident);
                        } else {
                            std::cout << "%" << last_ptr;
                        }
                        if(!num2.empty()) {
                            std::cout << ", " << num2 << std::endl;
                        } else {
                            std::cout << ", %" << global_reg - 1 << std::endl;
                        }
                        ++global_reg;
                    }
                    if(!num.empty()) {
                        std::cout << "  store " << num << ", %" << global_reg - 1 << std::endl;
                    } else {
                        std::cout << "  store %" << exp_reg << ", %" << global_reg - 1 << std::endl;
                    }
                } else if(sym.type == POINTER) {
                    std::cout << "  %" << global_reg << " = load @";
                    std::cout << lval_ptr->ident << "_" << symbol_table.query_scope(lval_ptr->ident) << std::endl;
                    ++global_reg;
                    for(int i = 0; i < lval_ptr->index_list->size(); ++i) {
                        last_ptr = global_reg - 1;
                        num2 = lval_ptr->index_list->at(i)->DumpIR();
                        if(i == 0) {
                            std::cout << "  %" << global_reg << " = getptr ";                    
                            std::cout << "%" << last_ptr;
                        } else {
                            std::cout << "  %" << global_reg << " = getelemptr ";
                            std::cout << "%" << last_ptr;
                        }
                        if(!num2.empty()) {
                            std::cout << ", " << num2 << std::endl;
                        } else {
                            std::cout << ", %" << global_reg - 1 << std::endl;
                        }
                        ++global_reg;
                    }
                    if(!num.empty()) {
                        std::cout << "  store " << num << ", %" << global_reg - 1 << std::endl;
                    } else {
                        std::cout << "  store %" << exp_reg << ", %" << global_reg - 1 << std::endl;
                    }                    
                }
                break;
            case EMPTY: break;
            case RETURN_EMPTY:
                std::cout << "  ret" << std::endl; 
                return "RETURN"; 
                break;
            case EXP: return exp->DumpIR(); break;
            case BLOCK: return block->DumpIR(); break;
            case IF:
                ++global_if;
                end_required = 1; // if语句一定需要end
                num = exp->DumpIR();
                if(!num.empty()) {
                    std::cout << "  br " << num << ", %then_" << cur_ifNo << ", %if_end_" << cur_ifNo << std::endl;
                } else {
                    std::cout << "  br " << "%" << global_reg - 1 << ", %then_" << cur_ifNo << ", %if_end_" << cur_ifNo << std::endl;
                }
                std::cout << std::endl;
                std::cout << "%then_" << cur_ifNo << ":\n";
                if(if_stmt->DumpIR() != "RETURN") {
                    std::cout << "  jump " << "%if_end_" << cur_ifNo << std::endl;
                }
                std::cout << std::endl;
                if(end_required) {
                    std::cout << "%if_end_" << cur_ifNo << ":\n";
                }
                return end_required ? "" : "RETURN";
                break; 
            case IFELSE:
                ++global_if;
                num = exp->DumpIR();
                if(!num.empty()) {
                    std::cout << "  br " << num << ", %then_" << cur_ifNo << ", %else_" << cur_ifNo << std::endl;
                } else {
                    std::cout << "  br " << "%" << global_reg - 1 << ", %then_" << cur_ifNo << ", %else_" << cur_ifNo << std::endl;
                }
                std::cout << std::endl;
                std::cout << "%then_" << cur_ifNo << ":\n";
                if(if_stmt->DumpIR() != "RETURN") {
                    std::cout << "  jump " << "%if_end_" << cur_ifNo << std::endl;
                    end_required = 1;
                }
                std::cout << std::endl;
                std::cout << "%else_" << cur_ifNo << ":\n";
                if(else_stmt->DumpIR() != "RETURN") {
                    std::cout << "  jump " << "%if_end_" << cur_ifNo << std::endl;    
                    end_required = 1;               
                }
                std::cout << std::endl;
                if(end_required) {
                    std::cout << "%if_end_" << cur_ifNo << ":\n";
                }
                return end_required ? "" : "RETURN";
                break;       
            case WHILE:
                old_while = global_curWhile;
                global_curWhile = global_whileCnt;
                ++global_whileCnt;
                while_fa[global_curWhile] = old_while;
                std::cout << "  jump %while_entry_" << global_curWhile << std::endl;
                std::cout << std::endl;
                std::cout << "%while_entry_" << global_curWhile << ":" << std::endl;
                num = exp->DumpIR();
                if(!num.empty()) {
                    std::cout << "  br " << num << ", %while_body_" << global_curWhile << ", %while_end_" << global_curWhile << std::endl;
                } else {
                    std::cout << "  br " << "%" << global_reg - 1 << ", %while_body_" << global_curWhile << ", %while_end_" << global_curWhile << std::endl;
                }
                std::cout << std::endl;
                std::cout << "%while_body_" << global_curWhile << ":" << std::endl;
                if(while_stmt->DumpIR() != "RETURN"){
                    std::cout << "  jump %while_entry_" << global_curWhile << std::endl;
                }
                std::cout << std::endl;
                std::cout << "%while_end_" << global_curWhile << ":" << std::endl;
                global_curWhile = while_fa[global_curWhile];
                return "WHILE_END";
                break;
            case BREAK:
                std::cout << "  jump %while_end_" << global_curWhile << std::endl;
                return "RETURN";
                break;
            case CONTINUE:
                std::cout << "  jump %while_entry_" << global_curWhile << std::endl;
                return "RETURN";
                break;          
            default: break;
        }
        return "";
    }

    int eval() const override {return 0;}
};

static inline void handle_aggregate(std::vector<std::pair<char,int>>& aggregate, std::vector<int>& lens, std::vector<int>& words, int pos, int cur, const std::string& ident, char mode) {
    if(mode == 'G') {
        if(cur == lens.size()) {
            std::cout << aggregate[pos].second;
        } else {
            std::cout << "{";
            int sz = words[cur] / lens[cur];
            for(int i = 0; i < lens[cur]; ++i) {
                handle_aggregate(aggregate, lens, words, pos + i * sz, cur + 1, ident, mode);
                if(i != lens[cur] - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "}";
        }
    } else {
        if(cur == lens.size()) {
            if(mode == 'C') {
                std::cout << "  store " << aggregate[pos].second << ", %" << global_reg - 1 <<std::endl;
            } else {
                if(aggregate[pos].first == 0) {
                    std::cout << "  store " << aggregate[pos].second << ", %" << global_reg - 1 << std::endl;   
                } else {
                    std::cout << "  store %" << aggregate[pos].second << ", %" << global_reg - 1 << std::endl; 
                }
            }
        } else {
            int sz = words[cur] / lens[cur];
            int ptr = global_reg - 1;
            for(int i = 0; i < lens[cur]; ++i) {
                std::cout << "  %" << global_reg << " = getelemptr ";
                if(cur == 0) {
                    std::cout << "@" << ident << "_" << symbol_table.query_scope(ident);
                } else {
                    std::cout << "%" << ptr;
                }
                std::cout << ", " << i << std::endl;
                ++global_reg;
                handle_aggregate(aggregate, lens, words, pos + i * sz, cur + 1, ident, mode);
            }
        }
    }
}