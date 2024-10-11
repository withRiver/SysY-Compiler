#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class BaseAST;
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;

class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;

/*
CompUnit  ::= FuncDef;

FuncDef   ::= FuncType IDENT "(" ")" Block;
FuncType  ::= "int";

Block     ::= "{" Stmt "}";
Stmt        ::= "return" Exp ";";



Exp         ::= LOrExp;
PrimaryExp  ::= "(" Exp ")" | Number;
Number      ::= INT_CONST;
UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
UnaryOp     ::= "+" | "-" | "!";
MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp     ::= EqExp | LAndExp "&&" EqExp;
LOrExp      ::= LAndExp | LOrExp "||" LAndExp;

*/

//使用到的寄存器编号
static int global_reg = 0;
//将字符op对应到其IR表示
static std::unordered_map<char, std::string> op2IR = {
    {'+', "add"}, 
    {'-', "sub"},
    {'*', "mul"},
    {'/', "div"},
    {'%', "mod"}
};

class BaseAST {
 public:
    virtual ~BaseAST() = default;    
    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
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
        func_def->DumpIR();
        std::cout << std::endl;
        //DumpIR结束，将reg重置为0
        global_reg = 0;
        return "";
    }
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
        block->DumpIR();
        return "";
    }
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
};

class BlockAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }

    std::string DumpIR() const override {
        std::cout << "{ \n" << "%entry: \n";
        stmt->DumpIR();
        std::cout << "}";
        return "";
    }
};

// Stmt ::= "return" Exp ";";
class StmtAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << "return ";
        exp->Dump();
        std::cout<<" }";
    }

    std::string DumpIR() const override {
        std::string num = exp->DumpIR();
        if(!num.empty()) {
            std::cout << "  ret " << num << std::endl;
        } else {
            std::cout << "  ret %" << global_reg - 1 << std::endl;
        }
        return "";
    }
};

//Exp ::= AddExp;
class ExpAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> add_exp;

    void Dump() const override {
        std::cout << "ExpAST { ";
        add_exp->Dump();
        std::cout << " }";
    }

    std::string DumpIR() const override {
        return add_exp->DumpIR();
    }
};

// PrimaryExp ::= "(" Exp ")" | Number;
class PrimaryExpAST : public BaseAST {
 public:
    enum TAG {BRAKET_EXP, NUMBER};
    TAG tag;
    std::unique_ptr<BaseAST> exp;
    int number;

    void Dump() const override {
        std::cout << "PrimaryExp { ";
        switch(tag) {
            case BRAKET_EXP:
                std::cout << "( ";
                exp->Dump();
                std::cout << " )";
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
                return exp->DumpIR();
                break;
            case NUMBER:
                //std::cout << "  %" << global_reg << " = add 0, ";
                //std::cout << number << std::endl;
                //++global_reg;
                return std::to_string(number);
                break;
            default:
                break;
        }
        return "";
    }
};

//UnaryExp ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST {
 public:
    enum TAG { PRIMARY_EXP, OP_UNARY_EXP};
    TAG tag;
    std::unique_ptr<BaseAST> primary_exp;
    char unary_op;
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
                if(unary_op == '+') {
                    if(!num.empty()) {
                        return num;
                    }
                    else {
                        ;
                    }
                }
                if(unary_op == '-') {
                    if(!num.empty()) {
                        std::cout << "  %" << global_reg << " = sub 0, " << num;
                    }
                    else {
                        std::cout << "  %" << global_reg << " = sub 0, %" << global_reg - 1;
                    }
                    std::cout << std::endl;
                    ++global_reg;
                }
                else if(unary_op == '!') {
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
};

// MulExp ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp ::= "*" | "/" | "%"
class MulExpAST : public BaseAST {
 public:
    enum Tag {UNARY_EXP, MULEXP_OP_UNARYEXP};
    Tag tag;
    std::unique_ptr<BaseAST> unary_exp;
    char mul_op;
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
            default:
                break;
        }
        return "";
    }

};

// AddExp ::= MulExp | AddExp AddOp MulExp;
// AddOp ::= "+" | "-"
class AddExpAST : public BaseAST {
 public:
    enum TAG {MUL_EXP, ADDEXP_OP_MULEXP};
    TAG tag;
    std::unique_ptr<BaseAST> mul_exp;
    char add_op;
    std::unique_ptr<BaseAST> add_exp;

    void Dump() const override {}
    
    std::string DumpIR() const override {
        std::string left_num, right_num;
        int left_reg, right_reg;
        switch(tag) {
            case MUL_EXP:
                return mul_exp->DumpIR();
                break;
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
            default:
                break;
        }
        return "";
    }
};