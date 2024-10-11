#pragma once

#include <iostream>
#include <memory>
#include <string>

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



Exp         ::= AddExp;
PrimaryExp  ::= "(" Exp ")" | Number;
Number      ::= INT_CONST;
UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
UnaryOp     ::= "+" | "-" | "!";
MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
*/

//使用到的寄存器编号
static int reg = 0;

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
        reg = 0;
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
            std::cout << "  ret %" << reg - 1 << std::endl;
        }
        return "";
    }
};

class ExpAST : public BaseAST {
 public:
    std::unique_ptr<BaseAST> unary_exp;

    void Dump() const override {
        std::cout << "ExpAST { ";
        unary_exp->Dump();
        std::cout << " }";
    }

    std::string DumpIR() const override {
        return unary_exp->DumpIR();
    }
};

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
                //std::cout << "  %" << reg << " = add 0, ";
                //std::cout << number << std::endl;
                //++reg;
                return std::to_string(number);
                break;
            default:
                break;
        }
        return "";
    }
};

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
                        std::cout << "  %" << reg << " = sub 0, " << num;
                    }
                    else {
                        std::cout << "  %" << reg << " = sub 0, %" << reg - 1;
                    }
                    std::cout << std::endl;
                    ++reg;
                }
                else if(unary_op == '!') {
                    if(!num.empty()) {
                        std::cout << "  %" << reg << " = eq 0, " << num; 
                    }
                    else {
                        std::cout <<"  %" << reg << " = eq 0, %" << reg - 1;
                    }
                    std::cout << std::endl;
                    ++reg;
                }
                break;
            default:
                break;
        }
        return "";
    }
};