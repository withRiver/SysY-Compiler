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

    void Dump() const override {}

    std::string DumpIR() const override {
        std::cout << "fun @" << ident << "(): "; 
        func_type->DumpIR();
        std::cout << "{\n";
        std::cout << "%LHR_entry_" << ident << ":\n";
        if(block->DumpIR() == "WHILE_END") {
            std::cout << "  ret 0" << std::endl;
        };
        std::cout << "}";
        std::cout << std::endl;
        return "";
    }

    int eval() const override {return 0;}
};

class FuncTypeAST : public BaseAST {
 public:
    std::string type = "int";

    void Dump() const override {}

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
                return right;
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
                return right;
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