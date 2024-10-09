#pragma once

#include <iostream>
#include <memory>
#include <string>



/*
CompUnit  ::= FuncDef;

FuncDef   ::= FuncType IDENT "(" ")" Block;
FuncType  ::= "int";

Block     ::= "{" Stmt "}";
Stmt      ::= "return" Number ";";
Number    ::= INT_CONST;
*/

class BaseAST {
 public:
    virtual ~BaseAST() = default;    
    virtual void Dump() const = 0;
    virtual void DumpIR(std::string& irStr) const = 0;
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

    void DumpIR(std::string& irStr) const override {
        func_def->DumpIR(irStr);
        irStr += "\n";
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

    void DumpIR(std::string& irStr) const override {
        irStr += "fun ";
        irStr +=  "@" + ident + "(): ";
        func_type->DumpIR(irStr);
        block->DumpIR(irStr);
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

    void DumpIR(std::string& irStr) const override {
        irStr += "i32 ";
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

    void DumpIR(std::string& irStr) const override {
        irStr += "{ \n";
        irStr += "%entry: \n";
        stmt->DumpIR(irStr);
        irStr += "\n}";
    }
};

class StmtAST : public BaseAST {
 public:
    int number;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout<<" }";
    }

    void DumpIR(std::string& irStr) const override {
        irStr += "ret ";
        irStr += std::to_string(number);
    }
};
