#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include "symbol_table.h"

class BaseAST;
class CompUnitAST;

class DeclAST;
class ConstDeclAST;
class ConstDefAST;
class ConstInitValAST;
class VarDeclAST;
class VarDefAST;
class InitValAST;

class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class BlockItemAST;
class StmtAST;

class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
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

class BaseAST {
 public:
    virtual ~BaseAST() = default;    
    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
    virtual int eval() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
        std::cout << std::endl;
    }

    std::string DumpIR() const override {
        //DumpIR开始时，将reg重置为0
        global_reg = 0;
        //DumpIR开始时, 初始化symbol_table
        symbol_table = SymbolTableList();
        symbol_table.init();
        func_def->DumpIR();
        return "";
    }

    int eval() const override {return 0;}
};

class FuncDefAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout<<" }";
    }

    std::string DumpIR() const override {
        std::cout << "fun @" << ident << "(): "; 
        func_type->DumpIR();
        std::cout << "{\n";
        std::cout << "%entry:\n";
        block->DumpIR();
        std::cout << "}";
        std::cout << std::endl;
        return "";
    }

    int eval() const override {return 0;}
};

class FuncTypeAST : public BaseAST {
 public:
    std::string type = "int";

    void Dump() const override {
        std::cout << "FuncTypeAST { ";
        std::cout << type;
        std::cout << " }";
    }

    std::string DumpIR() const override {
        std::cout << "i32 ";
        return "";
    }

    int eval() const override {return 0;}
};

// Block ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {
 public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> blockitem_vec;   

    void Dump() const override { }

    std::string DumpIR() const override {
        // 进入一个新的作用域
        symbol_table.enter_scope();
        int blockNo = 0;
        int index = 0;
        for(auto& blockitem : *blockitem_vec) {
            std::string str = blockitem->DumpIR();
            if(str == "RETURN" && index != blockitem_vec->size() - 1) {
                std::cout << "%entry" << ++blockNo << ":\n";    
            }
            ++index;
        }
        // 退出该作用域
        symbol_table.exit_scope();
        return "";
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

class BTypeAST : public BaseAST {
 public:
    void Dump() const override {}
    std::string DumpIR() const override {return "";}
    int eval() const override {return 0;}
};

// ConstDef ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST {
 public:
    std::string ident;
    std::unique_ptr<BaseAST> const_initval;
    void Dump() const override {}
    std::string DumpIR() const override {
        symbol_table.insert(ident, CONSTANT, const_initval->eval());
        return "";
    }
    int eval() const override {return 0;}
};

// ConstInitVal ::= ConstExp;
class ConstInitValAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> const_exp;
    void Dump() const override {}
    std::string DumpIR() const override {
        return const_exp->DumpIR();
    }
    int eval() const override {
        return const_exp->eval();
    }
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

// VarDef ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST {
 public:
    enum TAG {IDENT, IDENT_EQ_VAL};
    TAG tag;
    std::string ident;
    std::unique_ptr<BaseAST> initval;
    void Dump() const override {}
    std::string DumpIR() const override {
        // @x = alloc i32
        int scope_id = symbol_table.current_scope_id();
        std::string name = ident + "_" + std::to_string(scope_id);
        std::cout << "  @" << name << " = alloc i32\n";
        // store 10, @x
        if(tag == IDENT_EQ_VAL) {
            std::string num = initval->DumpIR();
            if(!num.empty()) {
                std::cout << "  store " << num << ", @" << name;
            } else {
                std::cout << "  store %" << global_reg - 1 << ", @" << name;
            }
            std::cout << std::endl;
        }
        symbol_table.insert(ident, VARIABLE, 0);
        return "";
    } 
    int eval() const override {return 0;}
};

// InitVal ::= Exp;
class InitValAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override {}
    std::string DumpIR() const override {
        return exp->DumpIR();
    }
    int eval() const override {
        return exp->eval();
    }
};

// LVal ::= IDENT
class LValAST : public BaseAST {
public:
    std::string ident;
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
            std::cout << "  %" << global_reg << " = load @" << ident <<"_" << scope;
            std::cout << std::endl;
            ++global_reg; 
        }
        return "";
    }
    int eval() const override {
        auto symb = symbol_table.query(ident);
        return symb->val;
    }
};

// Stmt ::= LVal "=" Exp ";" | [Exp] ";" | Block | "return" [Exp] ";";
class StmtAST : public BaseAST {
 public:
    enum TAG {ASSIGN, EMPTY, EXP, BLOCK, RETURN_EXP, RETURN_EMPTY};
    TAG tag;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << "return ";
        exp->Dump();
        std::cout<<" }";
    }

    std::string DumpIR() const override {
        std::string num;
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
                num = exp->DumpIR();
                if(!num.empty()) {
                    std::cout << "  store " << num << ", @";
                } else {
                    std::cout << "  store %" << global_reg - 1 <<", @";
                }
                std::cout << dynamic_cast<LValAST*>(lval.get())->ident;
                std::cout << "_" << symbol_table.query_scope(dynamic_cast<LValAST*>(lval.get())->ident);
                std::cout << std::endl;
                break;
            case EMPTY: break;
            case RETURN_EMPTY:
                std::cout << "  ret" << std::endl; 
                return "RETURN"; 
                break;
            case EXP: exp->DumpIR(); break;
            case BLOCK: block->DumpIR(); break;
            default: break;
        }

        return "";
    }

    int eval() const override {return 0;}
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

//UnaryExp ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST {
 public:
    enum TAG { PRIMARY_EXP, OP_UNARY_EXP};
    TAG tag;
    std::unique_ptr<BaseAST> primary_exp;
    std::string unary_op;
    std::unique_ptr<BaseAST> unary_exp;

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
class LAndExpAST : public BaseAST {
 public:
    enum TAG {EQ_EXP, LANDEXP_AND_EQEXP};
    TAG tag;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case EQ_EXP:
                return eq_exp->DumpIR();
                break;
            case LANDEXP_AND_EQEXP:
                left_num = land_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = eq_exp->DumpIR();
                right_reg = global_reg - 1;
                // A&&B = A & B = (A!=0) & (B!=0)
                std::cout << "  %" << global_reg << " = ne ";
                if(!left_num.empty()) {
                    std::cout << left_num << ", 0" << std::endl;
                } else {
                    std::cout << "%" << left_reg << ", 0" << std::endl;
                }
                left_reg = global_reg;
                ++global_reg;
                std::cout << "  %" << global_reg << " = ne ";
                if(!right_num.empty()) {
                    std::cout << right_num << ", 0" << std::endl;
                } else {
                    std::cout << "%" << right_reg << ", 0" <<std::endl;
                }
                right_reg = global_reg;
                ++global_reg;
                std::cout << "  %" << global_reg << " = and ";
                std::cout << "%" << left_reg <<", %" << right_reg;
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
            case EQ_EXP:
                return eq_exp->eval();
                break;
            case LANDEXP_AND_EQEXP:
                left = land_exp->eval();
                right = eq_exp->eval();
                return left && right;
                break;
            default: break;
        }
        return 0;
    }
};

// LOrExp  ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public BaseAST {
 public:
    enum TAG {LAND_EXP, LOREXP_OR_LANDEXP};
    TAG tag;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    void Dump() const override {}

    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case LAND_EXP:
                return land_exp->DumpIR();
                break;
            case LOREXP_OR_LANDEXP:
                left_num = lor_exp->DumpIR();
                left_reg = global_reg - 1;
                right_num = land_exp->DumpIR();
                right_reg = global_reg - 1;
                // A||B = A | B = (A!=0) | (B!=0)
                std::cout << "  %" << global_reg << " = ne ";
                if(!left_num.empty()) {
                    std::cout << left_num << ", 0" << std::endl;
                } else {
                    std::cout << "%" << left_reg << ", 0" << std::endl;
                }
                left_reg = global_reg;
                ++global_reg;
                std::cout << "  %" << global_reg << " = ne ";
                if(!right_num.empty()) {
                    std::cout << right_num << ", 0" << std::endl;
                } else {
                    std::cout << "%" << right_reg << ", 0" <<std::endl;
                }
                right_reg = global_reg;
                ++global_reg;
                std::cout << "  %" << global_reg << " = or ";
                std::cout << "%" << left_reg <<", %" << right_reg;
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
            case LAND_EXP:
                return land_exp->eval();
                break;
            case LOREXP_OR_LANDEXP:
                left = lor_exp->eval();
                right = land_exp->eval();
                return left || right;
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