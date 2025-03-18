# Simple Compiler

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 将SysY源代码翻译为Koopa IR。
2. 将SysY源代码翻译为RISC-V指令。
3. 正确编译C语言中的int类型、if-else分支结构、while循环结构、以及int或指针为参数的函数等等。

### 1.2 主要特点

本编译器的主要特点是**图灵完备的**、**用Koopa IR作为中间代码**。

## 二、编译器设计

### 2.1 主要模块组成

编译器由4个主要模块组成：sysy.l, sysy.y, AST.h, RISCV.h. sysy.l部分负责词法分析，将输入的代码转换为token流;
sysy.y部分负责语法分析，将各语法符号储存为对应的抽象语法树(AST); 
AST.h部分负责将AST转换为Koopa IR, 并进行必要的语义分析; RISCV.h部分负责将Koopa IR 转化为RISC-V机器指令。

### 2.2 主要数据结构

本编译器最核心的数据结构是各个非终结语法符号对应的AST结点。在实现过程中，对各种非终结语法符号都设计了其对应的AST结点数据结构，它们有共同的基类BaseAST: 

```c
class BaseAST {
 public:
    virtual ~BaseAST() = default;    
    virtual void Dump() const = 0;
    virtual std::string DumpIR() const = 0;
    virtual int eval() const = 0;
};
```

其中`DumpIR()`用于输出该结点对应的Koopa IR, `eval()`负责计算常量表达式的值。 对于有多个产生式的非终结符号，其AST结点会有一个枚举类型的变量来储存其对应哪一个产生式。

如果将一个SysY程序视作一棵树，那么一个`CompUnit`的实例就是这棵树的根，根据这一情况设计了数据结构`CompUnitAST`。

```c
class CompUnitAST : public BaseAST {
 public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> compunit_items;
}
```

它是输出Koopa IR的起点，`compunit_items`是程序中所有的全局变量定义或函数定义。

符号表可以记录作用域内所有被定义过的符号的信息，为此设计了数据结构`SymbolTable`。

```c
class SymbolTable {
    std::unordered_map<std::string, std::shared_ptr<Symbol>> table;
    std::shared_ptr<SymbolTable> prev;
    int scope_id;
    int insert(const std::string& ident, type_t type, int val);
    bool exist(const std::string& ident);
    std::shared_ptr<Symbol> query(const std::string& ident);
    int query_scope(const std::string& ident);
}
```

`table`是一个哈希表，用于储存标识符对应的符号类型、常量的值等信息。 由于涉及到作用域嵌套的问题，因此用`prev`来记录上一层作用域的符号表，`scope_id`是作用域的编号，全局作用域的`scope_id`为0，每有一个新的作用域，那么这个新作用域下符号表的`scope_id`就加1。 

`insert()`可以插入一个符号，`exist()`用于判断符号是否存在，`query()`用于查询一个符号的相关信息， `query_scope()`用于查询符号存在的最近作用域的`scope_id`。

我还设计了`stack_frame`来表示函数的栈帧。其主要功能为存储全局变量在栈帧中的偏移值。
```c
static std::unordered_map<koopa_raw_value_t, int> stack_frame;
```

此外，在数组的编译中，对于数组下标的处理，设计了数据结构`array_dims`和`array_ranges`。
它们是两个哈希表。

```c
static std::unordered_map<koopa_raw_value_t, std::vector<int>> array_dims;
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> range_t;
static std::unordered_map<koopa_raw_value_t, range_t> array_ranges;
```

`array_dims`记录每个数组value的各维度的长度，例如arr[2][5][8]对应的value在`array_dims`下的值就是{2,5,8}。 `array_ranges`记录每个value对应的是数组的哪些维度，这在处理getptr和getelemptr时非常有用，因为这些指令会"消耗"掉数组的一个维度。

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑
符号表在"2.2 主要数据结构"中已有讨论。符号表的主体部分就是一个标识符到符号信息的哈希表，标识符是一个字符串，符号信息包括符号的类型（常量、变量、函数、数组、指针），以及诸如常量的值，数组的维数等信息。对作用域的处理是采用对符号表的**链式**存储. 每个作用域都有一个符号表，每个符号表有一个前向指针指向上一个符号表，这样在退出当前作用域时通过前向指针就可回到上一个作用域。在查询符号时，也是沿着该前向指针一步步往前，直到找到第一个储存了该标识符的表项。

#### 2.3.2 寄存器分配策略
该编译器没有做寄存器分配，而是把所有的局部变量都放到栈上，并记录它们在栈上的偏移量。当需要使用这些局部变量时，用`lw t0, 偏移量(sp)`便可。

#### 2.3.3 采用的优化策略
做的唯一的小优化是对于整数字面值直接使用，而不需要事先将它存在一个局部变量中。相当于是常量传播的一小小小小部分。

#### 2.3.4 其它补充设计考虑
在DumpIR()函数中加入了类型为`std::string`的返回值，可以获取一些IR生成中的信息，例如`"RETURN"`表示这是一个返回语句，空串`""`
无需太多处理，对于整数字面值，则返回整数的值。

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成
```
主要是flex和bison的编写，以及AST数据结构的设计，和libkoopa的使用。

在flex中需要定义一些正则表达式去匹配源代码中的不同内容，并针对这些不同内容定义一些lexer的行为，例如存储标识符并返回token等等。

在bison中，按照文档里给的语法规则书写即可，在其中给AST.h文件中的AST结点类型的成员变量赋好值。这其实就是构建一整个AST的过程。

其中需要我们自己写块注释的正则表达式，如下：

[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]

```
#### Lv3. 表达式
```
优先级问题在语法规则中就已经解决好了，在bison中严格按照文档中给的语法规则进行语法分析即可实现正确的优先级，不需要额外处理。

对于表达式语句的翻译，每一次运算的值都用一个新的变量存储，这些变量为%0, %1, %2, ..., 用一个全局变量`global_reg`来记录目前使用了几个这样的变量。（变量名字用global_reg是因为我最开始写的时候错把%1这样的临时变量当成寄存器了，后面也不影响编程，所以也就不该了）。这样的话，%(global_reg) 就是当前存值可使用的编号，而%(global_reg-1)就是上一次运算的结果。每次使用%(global_reg)存值后都要给global_reg加1。

```
#### Lv4. 常量和变量
```
首先是增加了符号表，符号表的实现在前文已经详细说过了。

对于常量，给AST结点增加了`eval()`方法用于计算常量的值，它以递归的方式计算当前结点对应的常量的值。

对于变量，遇到是将其插入到符号表中。在目标代码的生成中，变量都放在栈上存储，要记录变量在栈中的位置。要给每个函数分配栈空间，栈帧的大小为变量数量*4并向上对齐到16。

```
#### Lv5. 语句块和作用域
```
作用域嵌套在符号表的设计中就实现好了。在遇到一个新的块时就创建一个新的符号表，在退出一个块时就通过符号表中的`prev`指针回到上一个作用域的符号表并删除该符号表。

由于Koopa IR中不允许存在同名变量，哪怕是在不同的作用域中的同名变量也不行，我想这是因为Koopa IR中并没有花括号`{}`来表示作用域。因此变量在IR中的的命名规则`变量名_{作用域编号}`，例如在编号为3的作用域中的名字为var的变量，在Koopa IR中的名字就是`var_3`。

此外，本部分在语法规则上出现了

`Block ::= "{" {BlockItem} "}"`

这样的规则，其中`{BlockItem}`可以用std::vector来存储。后面出现的`{}`同理。

还需要注意的是，一个语句块中可能有多个return语句。那么在第一个return后的语句就不应该出现在IR中了。这个可以通过`DumpIR()`的返回值实现，只要返回值是`"RETURN"`, 那么就停止输出该块后续项的IR。

```
#### Lv6. if语句
```
在 SysY 中, if/else 语句的 else 部分可有可无, 一旦出现了若干个 if 和一个 else 的组合, 在符合 EBNF 语法定义的前提下, 我们可以找到不止一种语法的推导 (或规约) 方法，于是这里产生了二义性问题。本编译器解决该问题的方法是在bison中处理if-else的部分使用%prec来让else必须和最近的if匹配。

这一部分还出现了label，依旧是使用全局变量来记录当前有多少个if语句，并以此给每个if语句块的label命名。

相关的跳转操作使用br, jump, 在IR和RISCV都有这样功能的指令。

这一部分还实现了短路求值，把 "lhs || rhs" 当作 "int result = 1; if(lhs == 0) result = rhs != 0;" 来进行翻译即可。

我在这一部分的目标代码生成中遇到了立即数大小溢出的问题。因为在`addi sp, sp, 立即数`中的立即数必须是12位有符号整数，如果超过了这一范围需要先将立即数加载到寄存器中，然后用对象是寄存器的add指令。

```
#### Lv7. while语句
```
在基本的跳转方面其实和if相似。主要是对于break和continue语句，其中break需要跳转到当前循环的结尾，continue需要跳转到当前循环的入口，因此我们需要直到“当前语句处于哪一个循环中”。为此，设计了全局变量`global_curWhile`来记录当前处于的while循环的标号，以及哈希表while_fa来记录循环对应的上一层循环。这样在进入循环时while_fa[global_curWhile] = old_while, 退出循环时global_chrWhile = while_fa[global_curWhile], 就可以在global_curWhile中维护当前处于哪一个循环中了。

在这里，我的设计可能会出现函数结尾没有ret的情况（结尾可能是%while_end:）。为此利用DumpIR()的返回信息，如果最后一句话返回的不是“RETURN”，那么就在末尾补一个ret或者ret 0。

```
#### Lv8. 函数和全局变量
```
全局变量位于0号作用域，这样能保证在所有作用域中都能向上找到全局变量。

函数的参数位于单独的作用域中，我们在进入函数后首先将参数加载到局部变量中，这可以方便目标代码的生成。

在目标代码部分，函数的第8个以上（如果存在）的参数需要在栈中储存，需要在栈的大小分配中考虑这部分。一个易错点在于被调用函数（callee）加载实参时，sp的偏移量要加上callee的栈帧长度，因为调用者（calller）将实参保存时的偏移量相对的是caller自己的栈指针的偏移量，并不是callee的（caller的栈帧位于更高地址）。偏移量加上callee的栈帧长度才能获取到存放在caller栈帧中的参数。

此外，在这一部分还出现了 FuncDef 和 VarDecl 的规约冲突，原因是FuncDef的FuncType和VarDecl的BType可以推导出相同的token，我们将FuncType与BType合并为同一个语法符号就解决了该冲突。

```
#### Lv9. 数组
```
主要难点在于IR生成部分的初始化列表处理。首先要补全初始化列表，例如将

int arr[2][3][4] = {1, 2, 3, 4, {5}, {6}, {7, 8}}补全为

int arr[2][3][4] = {
  {{1, 2, 3, 4}, {5, 0, 0, 0}, {6, 0, 0, 0}},
  {{7, 8, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
};

我们需要利用该数组各维度的总长以及各个维度对应的大小，例如int[2][3][4]的这两个分别是2,3,4和24,12,4。当遇到整数或表达式时，直接放入；如果是一个初始化列表，则要检查当前对齐到了哪一个边界, 然后将当前初始化列表视作这个边界所对应的最长维度的数组的初始化列表, 并递归处理。对于已经补全的初始化列表， 对初始化列表中的初始化列表再进行递归处理，直到递归到单个元素时做相应的store等操作。

在目标代码生成部分，难点在于当前加载到了数组的哪一个维度，以及计算下标i代表的实际偏移量。这用在数据结构部分已经提到的array_dims和array_ranges可以记录。此外，一个易错的地方在于如果alloc的值是一个指针的话（此时变量是一个指针的指针），那么需要再额外lw一次才能拿到我们想要的内容。

```

### 3.2 工具软件介绍
1. `flex`：词法分析。
2. `bison`： 语法分析。
3. `libkoopa`：解析Koopa IR, 将文本形式的Koopa IR 转换为 raw program。

### 3.3 测试情况说明

印象最深的是 `multiple_returns` 和 `立即数超范围的` 两个样例。在遇到第一个return时需要结束当前语句块；在`addi sp, sp, 立即数`中的立即数必须是12位有符号整数，如果超过了这一范围需要先将立即数加载到寄存器中。
