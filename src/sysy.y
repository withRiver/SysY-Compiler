%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "AST.h"
// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT UNARYADDOP MULOP RELOP EQOP
%token <int_val> INT_CONST
%token LAND LOR

// 非终结符的类型定义
%type <ast_val> CompUnitItem
%type <ast_val> Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal
%type <ast_val> FuncDef FuncFParam Block BlockItem Stmt 
%type <ast_val> Exp PrimaryExp UnaryExp FuncExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <int_val> Number
%type <ast_val> LVal
%type <vec_val> CompUnitItemList BlockItemList ConstDefList VarDefList
%type <vec_val> DimList IndexList ConstInitValList InitValList ConstArrayInitVal ArrayInitVal
%type <vec_val> FuncFParams FuncRParams
%type <vec_val> FuncFParamList FuncRParamList

// 此处参考了github上的代码, 用于解决if else 语句的优先级问题
%precedence IFX
%precedence ELSE

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnitItemList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->compunit_items = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    ast = move(comp_unit);
  }
  ;

CompUnitItemList 
  : CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitItemList CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  } 
  ;

CompUnitItem 
  : FuncDef {
    auto ast = new CompUnitItemAST();
    ast->funcdef_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitItemAST();
    ast->funcdef_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : BType IDENT '(' FuncFParams ')' Block {
    auto func_def = new FuncDefAST();
    func_def->func_type = unique_ptr<BaseAST>($1);
    func_def->ident = *unique_ptr<string>($2);
    func_def->func_fparams = unique_ptr<vector<unique_ptr<BaseAST>>>($4);
    func_def->block = unique_ptr<BaseAST>($6);
    $$ = func_def;
  }
  ;


FuncFParams
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | FuncFParamList {
    $$ = $1;
  } 
  ;

FuncFParamList 
  : FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  } 
  | FuncFParamList ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->tag = FuncFParamAST::INTEGER;
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' DimList {
    auto ast = new FuncFParamAST();
    ast->tag = FuncFParamAST::ARRAY;
    ast->btype = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->dim_list = unique_ptr<vector<unique_ptr<BaseAST>>>($5);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto block = new BlockAST();
    block->blockitem_vec = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = block;
  }
  ;

BlockItemList 
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  } 
  | BlockItemList BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->tag = BlockItemAST::DECL;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->tag = BlockItemAST::STMT;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::RETURN_EXP;
    stmt->exp = unique_ptr<BaseAST>($2);
    $$ = stmt;
  }
  | LVal '=' Exp ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::ASSIGN;
    stmt->lval = unique_ptr<BaseAST>($1);
    stmt->exp = unique_ptr<BaseAST>($3);
    $$ = stmt;
  }  
  | RETURN ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::RETURN_EMPTY;
    $$ = stmt;
  }
  | ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::EMPTY;
    $$ = stmt;
  }
  | Exp ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::EXP;
    stmt->exp = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  | Block {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::BLOCK;
    stmt->block = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  | IF '(' Exp ')' Stmt %prec IFX {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  } 
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IFELSE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::WHILE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->while_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::CONTINUE;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto decl = new DeclAST();
    decl->const_var_decl = unique_ptr<BaseAST>($1);
    $$ = decl;
  }
  | VarDecl {
    auto decl = new DeclAST();
    decl->const_var_decl = unique_ptr<BaseAST>($1);
    $$ = decl;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    ast->constdef_vec = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  | VOID {
    auto ast = new BTypeAST();
    ast->type = "void";
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT DimList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->dim_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    ast->const_initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

DimList
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | DimList '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->tag = ConstInitValAST::CONSTEXP;
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ConstArrayInitVal {
    auto ast = new ConstInitValAST();
    ast->tag = ConstInitValAST::CONSTINITVAL;
    ast->constinitval_list = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    $$ = ast;
  }
  ;

ConstArrayInitVal 
  : '{' '}' {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | '{' ConstInitValList '}' {
    $$ = $2;
  }

ConstInitValList 
  : ConstInitVal {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;   
  }
  ;

VarDecl 
  : BType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->vardef_vec = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

VarDefList 
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef 
  : IDENT DimList {
    auto ast = new VarDefAST();
    ast->tag = VarDefAST::IDENT;
    ast->ident = *unique_ptr<string>($1);
    ast->dim_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  | IDENT DimList '=' InitVal {
    auto ast = new VarDefAST();
    ast->tag = VarDefAST::IDENT_EQ_VAL;
    ast->ident = *unique_ptr<string>($1);
    ast->dim_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);    
    ast->initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->tag = InitValAST::EXP;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ArrayInitVal {
    auto ast = new InitValAST();
    ast->tag = InitValAST::INITVAL;
    ast->initval_list = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    $$ = ast;
  }
  ;

ArrayInitVal
  : '{' '}' {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | '{' InitValList '}' {
    $$ = $2;
  }
  ;

InitValList 
  : InitVal {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValList ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;   
  }
  ;

Number
  : INT_CONST {
    $$ = $1; 
  }
  ;

LVal
  : IDENT IndexList {
    auto lval = new LValAST();
    lval->ident = *unique_ptr<string>($1);
    lval->index_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = lval;
  }

IndexList
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | IndexList '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

Exp
  : LOrExp {
    auto exp = new ExpAST();
    exp->lor_exp = unique_ptr<BaseAST>($1);
    $$ = exp;
  }
  ;

PrimaryExp 
  : '(' Exp ')' {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->tag = PrimaryExpAST::BRAKET_EXP;
    primary_exp->exp = unique_ptr<BaseAST>($2);
    $$ = primary_exp;
  } 
  | LVal {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->tag = PrimaryExpAST::LVAL;
    primary_exp->lval = unique_ptr<BaseAST>($1);
    $$ = primary_exp;
  }
  | Number {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->tag = PrimaryExpAST::NUMBER;
    primary_exp->number = $1;
    $$ = primary_exp;
  }
  ;

UnaryExp 
  : PrimaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->tag = UnaryExpAST::PRIMARY_EXP;
    unary_exp->primary_exp = unique_ptr<BaseAST>($1);
    $$ = unary_exp;
  }
  | UNARYADDOP UnaryExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->tag = UnaryExpAST::OP_UNARY_EXP;
    unary_exp->unary_op = *unique_ptr<string>($1);
    unary_exp->unary_exp = unique_ptr<BaseAST>($2);
    $$ = unary_exp;
  }
  | FuncExp {
    auto unary_exp = new UnaryExpAST();
    unary_exp->tag = UnaryExpAST::FUNC_EXP;
    unary_exp->func_exp = unique_ptr<BaseAST>($1);
    $$ = unary_exp;
  }
  ;

FuncExp 
  : IDENT '(' FuncRParams ')' {
    auto ast = new FuncExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_rparams = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;

FuncRParams
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | FuncRParamList {
    $$ = $1;
  }
  ;
FuncRParamList
  : Exp {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParamList ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;


MulExp 
  : UnaryExp {
    auto mul_exp = new MulExpAST();
    mul_exp->tag = MulExpAST::UNARY_EXP;
    mul_exp->unary_exp = unique_ptr<BaseAST>($1);
    $$ = mul_exp;
  }
  | MulExp MULOP UnaryExp {
    auto mul_exp = new MulExpAST();
    mul_exp->tag = MulExpAST::MULEXP_OP_UNARYEXP;
    mul_exp->mul_exp = unique_ptr<BaseAST>($1);
    mul_exp->mul_op = *unique_ptr<string>($2);
    mul_exp->unary_exp = unique_ptr<BaseAST>($3);
    $$ = mul_exp;
  }
  ;

AddExp
  : MulExp {
    auto add_exp = new AddExpAST();
    add_exp->tag = AddExpAST::MUL_EXP;
    add_exp->mul_exp = unique_ptr<BaseAST>($1);
    $$ = add_exp;
  }
  | AddExp UNARYADDOP MulExp {
    auto add_exp = new AddExpAST();
    add_exp->tag = AddExpAST::ADDEXP_OP_MULEXP;
    add_exp->add_exp = unique_ptr<BaseAST>($1);
    add_exp->add_op = *unique_ptr<string>($2);
    add_exp->mul_exp = unique_ptr<BaseAST>($3);
    $$ = add_exp;
  }
  ;

RelExp
  : AddExp {
    auto rel_exp = new RelExpAST();
    rel_exp->tag = RelExpAST::ADD_EXP;
    rel_exp->add_exp = unique_ptr<BaseAST>($1);
    $$ = rel_exp;
  }
  | RelExp RELOP AddExp {
    auto rel_exp = new RelExpAST();
    rel_exp->tag = RelExpAST::RELEXP_OP_ADDEXP;
    rel_exp->rel_exp = unique_ptr<BaseAST>($1);
    rel_exp->rel_op = *unique_ptr<string>($2);
    rel_exp->add_exp = unique_ptr<BaseAST>($3);
    $$ = rel_exp;
  }
  ;

EqExp
  : RelExp {
    auto eq_exp = new EqExpAST();
    eq_exp->tag = EqExpAST::REL_EXP;
    eq_exp->rel_exp = unique_ptr<BaseAST>($1);
    $$ = eq_exp;
  }
  | EqExp EQOP RelExp {
    auto eq_exp = new EqExpAST();
    eq_exp->tag = EqExpAST::EQEXP_OP_RELEXP;
    eq_exp->eq_exp = unique_ptr<BaseAST>($1);
    eq_exp->eq_op = *unique_ptr<string>($2);
    eq_exp->rel_exp = unique_ptr<BaseAST>($3);
    $$ = eq_exp;
  }
  ;

LAndExp
  : EqExp {
    auto land_exp = new LAndExpAST();
    land_exp->tag = LAndExpAST::EQ_EXP;
    land_exp->eq_exp = unique_ptr<BaseAST>($1);
    $$ = land_exp;
  }
  | LAndExp LAND EqExp {
    auto land_exp = new LAndExpAST();
    land_exp->tag = LAndExpAST:: LANDEXP_AND_EQEXP;
    land_exp->land_exp = unique_ptr<BaseAST>($1);
    land_exp->eq_exp = unique_ptr<BaseAST>($3);
    $$ = land_exp;
  }
  ;

LOrExp
  : LAndExp {
    auto lor_exp = new LOrExpAST();
    lor_exp->tag = LOrExpAST::LAND_EXP;
    lor_exp->land_exp = unique_ptr<BaseAST>($1);
    $$ = lor_exp;
  }
  | LOrExp LOR LAndExp {
    auto lor_exp = new LOrExpAST();
    lor_exp->tag = LOrExpAST::LOREXP_OR_LANDEXP;
    lor_exp->lor_exp = unique_ptr<BaseAST>($1);
    lor_exp->land_exp = unique_ptr<BaseAST>($3);
    $$ = lor_exp;
  }
  ;

ConstExp
  : Exp {
    auto ast=new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
