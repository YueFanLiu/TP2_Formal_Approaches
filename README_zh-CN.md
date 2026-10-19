# TP2 零基础中文阅读指南

这份文档帮助你从头理解我们自己的 TP2。你可以一边阅读，一边打开代码和运行例子，不需要一次读完。

原来的 `README.md` 保留不动。本文件是额外的中文学习指南，不是课程提交报告。

如果你现在只想让程序运行起来，先看第 3 节；如果你想理解程序为什么这样写，从第 1 节开始。

## 目录

1. [我们到底在做什么](#1-我们到底在做什么)
2. [先认识几个词](#2-先认识几个词)
3. [先运行一次](#3-先运行一次)
4. [文件分别负责什么](#4-文件分别负责什么)
5. [跟着一个例子走完整个流程](#5-跟着一个例子走完整个流程)
6. [Scanner 怎样保存 Token](#6-scanner-怎样保存-token)
7. [Parser 怎样一步步读取 Token](#7-parser-怎样一步步读取-token)
8. [为什么要有 expr、term、factor 三个函数](#8-为什么要有-exprtermfactor-三个函数)
9. [AST 在 C++ 代码里是什么样的](#9-ast-在-c-代码里是什么样的)
10. [各种语句如何被解析](#10-各种语句如何被解析)
11. [DFS 是怎样打印树的](#11-dfs-是怎样打印树的)
12. [报错怎么看](#12-报错怎么看)
13. [测试怎么运行](#13-测试怎么运行)
14. [初学者常见的 C++ 写法](#14-初学者常见的-c-写法)
15. [建议的学习顺序与小练习](#15-建议的学习顺序与小练习)
16. [当前项目的能力边界](#16-当前项目的能力边界)

## 1. 我们到底在做什么

整个学期的目标是做一个小型编译器，把一种简化的 C++ 风格语言转换成 NASM 汇编。

这次 TP2 完成的是其中的一段：读取代码，检查它是否符合我们定义的语法，再把程序结构保存为一棵树。

例如，输入文件可以包含：

```cpp
int a;
a = 2 + 3 * 4;
print(a);
```

我们的程序会识别“声明 a”“给 a 赋值”“输出 a”这三条语句，并显示它们的结构。

**当前运行 `./compiler`，输出的是程序结构，而不是变量的计算结果。** 所以上面的输入不会直接输出 14。

要区分两个不同的东西：

| 名称 | 指的是什么 |
|---|---|
| 我们编写的工具 | src/ 里的 C++ 代码，编译后得到 compiler |
| 工具读取的输入 | read.txt 等文件里的简化语言代码 |

我们用真正的 C++ 写了一个工具，让它能够“读懂”我们自己定义的简化语言。

## 2. 先认识几个词

| 词 | 中文理解 | 本项目中的例子 |
|---|---|---|
| Grammar | 语法，规定什么写法合法 | 赋值必须写成“变量 = 表达式 ;” |
| EBNF | 描述语法规则的一种记法 | Assignment = AssignmentCore ";" ; |
| Scanner / Lexer | 词法分析器，把字符分成有类别的小单位 | 把 a = 2; 分成 a、=、2、; |
| Token | 一个有类别的小单位 | “2”是数字；“=”是赋值符号 |
| Parser | 语法分析器，检查这些单位的组合是否符合规则 | 检查等号后有没有表达式 |
| AST | 抽象语法树，保存代码的结构 | 赋值节点包含目标变量和右侧表达式 |
| Node | 树上的一个节点 | 数字节点、加法节点、循环节点 |
| DFS | 深度优先遍历，一种访问树节点的顺序 | 先输出一个节点，再访问它的子节点 |

整个流程是：

```text
输入文件，例如 read.txt
          |
          v
Scanner：识别每个词和符号
          |
          v
Token 列表：按顺序保存识别结果
          |
          v
Parser：按语法规则读取 Token，并构建 AST
          |
          v
AST：保存程序结构
          |
          v
DFS：在终端打印 AST
```

Scanner 和 Parser 的区别尤其重要。例如 `a = ;` 中，每个字符都能被 Scanner 识别，但是等号后缺少表达式，因此 Parser 会报错。

## 3. 先运行一次

### 3.1 在哪里输入命令

下面的编译和运行命令都要在 **WSL Ubuntu 终端** 中执行。

如果你现在打开的是 Windows PowerShell，可以先输入：

```powershell
wsl -d Ubuntu
```

进入 Ubuntu 后再执行下面的命令。也可以直接从开始菜单打开 Ubuntu。

Windows 的 `D:\TP2_Formal_Approaches` 在 WSL 里对应 `/mnt/d/TP2_Formal_Approaches`。

### 3.2 一次运行项目

当前项目已经有 build 文件夹。下面这组命令无论 build 是否存在都可以使用：

```bash
cd /mnt/d/TP2_Formal_Approaches
cmake -S . -B build
cmake --build build
cd build
./compiler
```

每条命令的作用：

| 命令 | 作用 |
|---|---|
| cd /mnt/d/TP2_Formal_Approaches | 进入项目文件夹 |
| cmake -S . -B build | 读取当前项目的配置，在 build 中准备编译文件 |
| cmake --build build | 编译 C++ 代码，生成 compiler |
| cd build | 进入编译输出文件夹 |
| ./compiler | 运行当前文件夹中的 compiler 程序 |

这里的 `.` 表示当前文件夹，`..` 表示上一层文件夹，`./` 表示“当前文件夹里的”。

不带参数的 `./compiler` 会读取 `../read.txt`。因此，这种运行方式需要先进入项目的 build 文件夹。

### 3.3 先看最简单的例子

在 build 文件夹里运行：

```bash
./compiler ../examples/01_expressions.txt
```

输入文件是：

```cpp
int a, b;
a = 2 + 3 * 4;
b = (2 + 3) * 4;
```

对应的实际输出是：

```text
Program
  Declaration: int
    Identifier: a
    Identifier: b
  Assignment
    Identifier: a
    BinaryOp: +
      Number: 2
      BinaryOp: *
        Number: 3
        Number: 4
  Assignment
    Identifier: b
    BinaryOp: *
      BinaryOp: +
        Number: 2
        Number: 3
      Number: 4
```

缩进越深，表示节点在树中的层级越深。Number: 3 和 Number: 4 都属于乘法节点。

### 3.4 改了文件后需要做什么

- 修改 read.txt 或其他输入文件：直接重新运行 compiler 即可。
- 修改 src/ 或 include/ 中的代码：先重新编译，再运行。
- 修改 CMakeLists.txt：重新执行 CMake 配置和编译。

例如，从项目根目录重新编译：

```bash
cd /mnt/d/TP2_Formal_Approaches
cmake --build build
cd build
./compiler
```

编译需要 g++、CMake、Flex 和 make；当前 WSL Ubuntu 中已安装。如果换到新机器，需要先安装这些工具。

### 3.5 老师要求的命令

在一个还没有 build 文件夹的项目副本中，从项目根目录执行：

```bash
mkdir build && cd build
cmake ..
cmake --build .
./compiler
```

`&&` 表示前一条命令成功后才执行后一条。如果提示 build 已存在，直接 `cd build` 即可，不需要为了重跑删除文件夹。

## 4. 文件分别负责什么

```text
TP2_Formal_Approaches/
├── CMakeLists.txt              编译配置
├── GrammerDefinition.txt       你们 TP1 的 EBNF 语法，原样保留
├── README.md                   原有说明
├── README_zh-CN.md             当前这份中文学习指南
├── read.txt                    默认输入，包含完整语法示例
├── include/
│   ├── tokens.hpp              Token 的种类与数据结构
│   ├── scanner.hpp             Scanner 类的声明与保存 Token 的方法
│   ├── parser.hpp              Parser 类的声明
│   └── ast.hpp                 AST 节点的定义
├── src/
│   ├── scanner.l               Flex 的匹配规则与动作
│   ├── main.cpp                程序入口，把各组件连接起来
│   ├── parser.cpp              语法分析与 AST 构建
│   └── ast.cpp                 各种 AST 节点的打印方法
├── examples/
│   ├── 01_expressions.txt      最简单的表达式示例
│   └── tp1_input.txt           你们 TP1 的综合输入示例
├── tests/
│   └── test_parser.py          自动测试
└── build/                     编译生成的文件，不是主要阅读对象
```

`.hpp` 一般放类型、类和函数的声明；`.cpp` 一般放具体实现。比如 parser.hpp 列出 Parser 有哪些方法，parser.cpp 写出这些方法怎样工作。

scanner.l 是 Flex 的规则文件，编译时会生成 build/scanner.cpp。要改词法规则，应修改 src/scanner.l；生成的 scanner.cpp 不适合作为入门阅读材料，也不应手工修改。

## 5. 跟着一个例子走完整个流程

我们只看这一行：

```cpp
a = 2 + 3 * 4;
```

为了专注理解语法，这里先不写声明。当前 Parser 不检查变量是否已经声明，所以这行可以单独被解析。

### 5.1 Scanner 看到什么

它会保存以下 Token。假设代码在第 1 行：

| 顺序 | type：类别 | value：原文 | line：行号 |
|---|---|---|---|
| 0 | Identifier | a | 1 |
| 1 | Assign | = | 1 |
| 2 | Number | 2 | 1 |
| 3 | Arithmetic | + | 1 |
| 4 | Number | 3 | 1 |
| 5 | Arithmetic | * | 1 |
| 6 | Number | 4 | 1 |
| 7 | Semicolon | ; | 1 |
| 8 | End | <EOF> | 1 |

空格不会变成 Token。<EOF> 是扫描结束后程序额外添加的结束标记，不是你写在文件里的文本。

### 5.2 Parser 怎样判断这是一条赋值语句

它从列表开头读取：

1. 第一个 Token 是变量名 a，所以 statement() 选择 assignment()。
2. assignment() 调用 assignment_core()。
3. assignment_core() 读取变量名，再检查后面是不是等号。
4. 调用 expr()，读取并构建等号右边的表达式树。
5. assignment() 最后检查分号。
6. 把得到的赋值节点加入整个 Program 的语句列表。

### 5.3 构建出来的树

```text
Program
  Assignment
    Identifier: a
    BinaryOp: +
      Number: 2
      BinaryOp: *
        Number: 3
        Number: 4
```

Assignment 的两个主要部分是“赋给谁”和“右边是什么表达式”。

BinaryOp 是 binary operation，表示有两个操作数的运算。这里加法的左边是数字 2，右边是一整个乘法表达式。

这棵树表达了运算关系；程序此时没有计算 3 * 4，也没有把值保存到变量 a。

### 5.4 main.cpp 负责连接这些步骤

核心代码是：

```cpp
auto tokens = scan_tokens(*input);
Parser parser(std::move(tokens));
auto root = parser.get_root();
print_ast(*root, std::cout);
```

逐行理解：

| 代码 | 含义 |
|---|---|
| scan_tokens(*input) | 扫描输入，得到 Token 列表 |
| Parser parser(...) | 创建一个 Parser，把列表交给它 |
| parser.get_root() | 解析整个输入，得到 AST 的根节点 |
| print_ast(...) | 从根节点开始，打印整棵树 |

## 6. Scanner 怎样保存 Token

### 6.1 一个 Token 是一个小记录

在 include/tokens.hpp 中：

```cpp
struct Token {
    TokenType type;
    std::string value;
    int line;
};
```

struct 把有关联的信息放在一起。例如数字 12 在第 3 行出现时，可表示为：

```cpp
Token{TokenType::Number, "12", 3}
```

这里的 "12" 仍是字符串，表示源代码中的原文，而不是正在参与 C++ 计算的整数 12。

### 6.2 vector 是按顺序存放 Token 的列表

```cpp
std::vector<Token> tokens;
```

可以把它理解为一个能够增长的数组。push_back(...) 会把一个新元素加到列表末尾。

在 include/scanner.hpp 中，add() 的核心是：

```cpp
tokens_.push_back(Token{type, text, lineno()});
```

这行代码的意思是：把类别、识别到的文字和当前行号打包，追加到列表中。

### 6.3 Flex 规则由“匹配什么”和“做什么”组成

例如 src/scanner.l 中：

```text
"int"       { add(TokenType::Int, yytext); }
{NUMBER}    { add(TokenType::Number, yytext); }
```

左边是匹配规则，右边花括号里的代码是匹配成功后执行的动作。yytext 是 Flex 提供的“刚刚匹配到的原文”。

NUMBER 的定义是：

```text
DIGIT       [0-9]
NUMBER      {DIGIT}+
```

意思是 NUMBER 由一个或多个数字组成。这里的 `+` 是正则表达式的重复记号，不是我们语言里的加法运算。

Flex 会优先选择能匹配最多字符的规则；长度相同时选择写在前面的规则。因此关键字规则在变量规则前面，int 会被识别为关键字，而 intValue 仍然是一个完整变量名。

### 6.4 TP1 到 TP2 的核心变化

TP1 识别 Token 后把它打印出来，方便人查看。TP2 把 Token 存起来，交给 Parser 使用。

匹配规则大部分保持不变，改变的是匹配成功后的动作，以及由 main.cpp 统一启动整个程序。

## 7. Parser 怎样一步步读取 Token

### 7.1 pos_ 记录目前读到哪里

Parser 保存了整个 Token 列表，以及当前位置 pos_。刚开始 pos_ = 0，表示正在看第一个 Token。

```text
Token： a    =    2    +    3    *    4    ;    <EOF>
下标：  0    1    2    3    4    5    6    7      8
        ^
      pos_
```

current() 查看当前 Token，但不会移动位置。

consume(...) 检查当前 Token 是否符合要求。符合时取出它并把位置向后移一格；不符合时报告错误。

例如：

```cpp
consume(TokenType::Assign, "'='");
```

意思是：“接下来必须是赋值号；如果确实是，就读过去。” 第二个参数用来构造错误提示。

### 7.2 get_root() 负责解析整个程序

它创建一个 Program 节点，然后重复调用 statement()，直到遇到结束标记 End。

可以用下面的思路理解：

```text
创建空的 Program
只要还没有读到文件末尾：
    解析一条语句
    把它存进 Program
返回 Program
```

### 7.3 statement() 根据开头选择语句类型

| 当前 Token | 调用哪个函数 |
|---|---|
| Int | declarations()，解析声明 |
| Identifier | assignment()，解析赋值 |
| Print | printing()，解析输出语句 |
| If | if_statement()，解析条件分支 |
| While | while_statement()，解析 while 循环 |
| For | for_statement()，解析 for 循环 |
| Switch | switch_statement()，解析 switch |

这就是当前语法下“看第一个词，决定接下来按哪条规则读”的过程。

### 7.4 什么叫递归下降

“递归”就是函数在处理较小部分时，可能再次调用自己，或者通过其他函数再调用到自己。

例如解析括号时：

```text
expr() → term() → factor()
                     |
                     | 遇到左括号
                     v
                   expr()
```

括号里面还是一个表达式，所以可以再次使用 expr()。这让同一套逻辑能够处理一层、两层甚至更多层括号。

“下降”指从较大的规则开始，逐层处理它包含的小规则：程序 → 语句 → 表达式 → 数字或变量。

## 8. 为什么要有 expr、term、factor 三个函数

这三个名字对应三个层次：

| 函数 | 负责什么 | 例子 |
|---|---|---|
| expr() | 把加减连接起来 | a + b - c |
| term() | 把乘除连接起来 | a * b / c |
| factor() | 处理数字、变量或括号里的表达式 | 12、a、(a + b) |

### 8.1 用 2 + 3 * 4 追踪调用

1. expr() 先让 term() 读取左边的部分。
2. term() 让 factor() 读取数字 2。
3. 数字 2 后面是加号，term() 不负责加法，所以返回数字节点 2。
4. expr() 看到加号，记住它，再调用 term() 读取右边。
5. 这次 term() 先读取 3，接着看见乘号，于是继续读取 4，组成乘法节点。
6. term() 返回整棵乘法子树。
7. expr() 把数字 2、加号和乘法子树组合为加法节点。

因此，不是简单地按字符从左到右把所有符号放在同一级，而是先通过函数层次组成高优先级的部分。

### 8.2 括号为什么能改变优先级

对于 `(2 + 3) * 4`，factor() 遇到左括号后，会调用 expr() 完整解析括号里的 2 + 3，再检查右括号。

这样，加法子树就成为外部乘法的左操作数。

### 8.3 左结合是什么意思

`20 - 5 - 2` 按规则应该理解为 `(20 - 5) - 2`。

expr() 每读到一个新运算符，就把已经构建的树放到新节点的左边，因此实现左结合。除法也通过 term() 里的同类循环实现左结合。

## 9. AST 在 C++ 代码里是什么样的

### 9.1 所有节点都有共同的基础类型 Node

include/ast.hpp 中定义了 Node，以及多种具体节点。可以理解为：Node 是总类别，数字、变量、赋值、循环等是具体类别。

例如：

```cpp
struct BinOpNode : Node {
    NodePtr left;
    Token op;
    NodePtr right;
    // 构造与打印方法见实际代码
};
```

这是字段示意，完整定义请看实际文件。三个字段分别表示左边的子树、运算符和右边的子树。

冒号后面的 Node 表示 BinOpNode 继承自 Node，也就是“二元运算节点也是一种节点”。

### 9.2 NodePtr 表示指向节点的智能指针

```cpp
using NodePtr = std::shared_ptr<Node>;
```

你可以先把 NodePtr 理解成“找到另一个节点的引用关系”。树通过这些关系把节点连接起来。

shared_ptr 会管理节点的生存期。当前树结构里，当节点不再被任何 shared_ptr 持有时，相关对象会自动释放，不需要手工 delete。

### 9.3 make_shared 创建新节点

```cpp
std::make_shared<NumberNode>(token)
```

表示创建一个数字节点，交给它一个 Token，并返回管理这个节点的智能指针。

在 expr() 中：

```cpp
left = std::make_shared<BinOpNode>(left, std::move(op), std::move(right));
```

表示用已有的左子树、运算符和右子树建立一个新的二元运算节点，再把新树记为 left。

### 9.4 常见节点对应什么

| 节点 | 保存的主要信息 |
|---|---|
| Program | 整个程序中的语句列表 |
| Block | 一个语句块中的语句列表 |
| NumberNode | 数字的 Token |
| IdentifierNode | 变量名的 Token |
| StringNode | 字符串的 Token |
| BinOpNode | 左子树、运算符、右子树 |
| Declaration | 声明的变量名列表 |
| Assignment | 目标变量、右侧表达式 |
| Condition | 两侧表达式和比较符号 |
| PrintStatement | 输出参数列表 |
| IfStatement | 条件、then 语句块、可选的 else 语句块 |
| WhileStatement | 条件与循环体 |
| ForStatement | 初始化、条件、更新、循环体 |
| SwitchStatement | 被判断的变量与有序分支列表 |
| SwitchBranch | case 数字或 default 标记，以及分支中的语句 |
| BreakStatement | 分支末尾的 break |

## 10. 各种语句如何被解析

### 10.1 先认识 EBNF 的符号

你们的 GrammerDefinition.txt 使用 EBNF：

| 写法 | 含义 |
|---|---|
| A = ... ; | 定义规则 A；末尾分号结束这条语法定义 |
| "int" | 源代码里要实际出现的文字 int |
| A \| B | A 或 B |
| [ A ] | A 可以出现，也可以省略 |
| { A } | A 可以重复零次或多次 |

EBNF 的 `{ Statement }` 表示“重复的语句”，不是要求源文件写一对花括号；`"{"` 和 `"}"` 才表示源文件中的实际花括号。

### 10.2 声明与赋值

```cpp
int a, b;
a = 10;
```

declarations() 先检查 int，再读取一个变量名，然后处理零个或多个“逗号 + 变量名”，最后检查分号。

assignment_core() 读取“变量名 = 表达式”。assignment() 在这个基础上再检查分号。

你们当前语法把声明和赋值分开，所以 `int a = 10;` 会报错。

### 10.3 print

```cpp
print("value", a);
```

printing() 检查 print 和左括号，读取至少一个参数，再处理逗号后的其他参数，最后检查右括号和分号。

print_argument() 只接受变量或字符串。因此 print()、print(123) 和 print(a + b) 都不属于当前语法。

注意：Parser 会创建 Print 节点。它不会在解析时执行这条输出语句。

### 10.4 条件与 if/else

```cpp
if (a > b) {
    print(a);
} else {
    print(b);
}
```

condition() 读取“表达式 + 比较符号 + 表达式”。if_statement() 再读取花括号包围的语句块，并检查后面有没有 else。

没有 else 时，AST 中的 else_body 为空；如果写了 `else {}`，else_body 是存在的空语句块。这两种情况可以区分。

### 10.5 while 与嵌套语句块

```cpp
while (a > 0) {
    a = a - 1;
}
```

while_statement() 读取条件和循环体。block() 在花括号内重复调用 statement()。

因为 statement() 又能解析 if、while 等结构，所以循环里可以包含条件分支，条件分支里也可以包含循环。

AST 只是保存循环结构。即使输入写的是 `for (;;) {}`，当前工具也不会因此真的进入无限循环。

### 10.6 for 为什么使用 AssignmentCore

```cpp
for (i = 0; i < 3; i = i + 1) {
    print(i);
}
```

头部由“初始化；条件；更新”组成。两个分号归 for_statement() 负责，初始化和更新使用不带分号的 assignment_core()。

否则，如果更新部分复用普通 assignment()，它会在右括号前再要求一个分号，错误地变成 `i = i + 1;)`。

三个部分都可以省略，所以 `for (;;) {}` 合法。打印 AST 时，省略的部分显示为 `<empty>`。

### 10.7 switch 与 break

```cpp
switch (a) {
    case 0:
        print("zero");
        break;
    default:
        print("other");
}
```

switch_statement() 读取变量名，随后读取 case 分支和可选的 default。switch_branch() 读取每个分支中的语句。

当前语法要求：case 后面是无符号整数形式的数字；default 最多一个且必须在最后；break 可省略，但如果出现，只能放在该分支末尾。

因此，普通 while 循环里的 break、分支内部 if 中的 break、break 后继续写普通语句，都不符合我们当前的语法。这是你们简化语言的范围，不等同于完整 C++。

## 11. DFS 是怎样打印树的

DFS 是 Depth-First Search，即深度优先遍历。本项目采用前序方式：先打印当前节点，再依次完整打印每个子节点。

例如：

```text
BinaryOp: +
  Number: 2
  BinaryOp: *
    Number: 3
    Number: 4
```

访问顺序是“加法 → 2 → 乘法 → 3 → 4”。

src/ast.cpp 中二元运算的打印代码是：

```cpp
void BinOpNode::print(std::ostream& out, int depth) const {
    print_line(out, depth, "BinaryOp: " + op.value);
    left->print(out, depth + 1);
    right->print(out, depth + 1);
}
```

depth 表示当前深度。每进入一层子节点就加 1，print_line() 用“深度 × 2 个空格”显示层次。

打印过程中，分号、括号不需要作为独立节点输出：Parser 已经检查过它们，树的层次也已经保留了括号表达的分组关系。

## 12. 报错怎么看

### 12.1 词法错误

输入：

```cpp
a = @;
```

提示类似：

```text
Line 1: unknown character '@'
```

意思是 Scanner 不认识字符 @。引号没有闭合的字符串也会在扫描阶段报错。

### 12.2 语法错误

输入：

```cpp
a = ;
```

提示：

```text
Line 1: expected a number, identifier or '('; got ';'
```

翻译是：“第 1 行，本来应该读到数字、变量名或左括号，实际却读到了分号。”

另一个例子是漏写分号。Parser 可能到下一行或文件末尾才发现错误，所以提示的位置是它发现不匹配的位置，不一定就是最初漏写字符的位置。

### 12.3 找不到文件

如果出现：

```text
Cannot open file: ../read.txt
```

通常先检查当前目录：

```bash
pwd
ls
```

pwd 显示当前在哪个文件夹；ls 列出其中的文件。默认运行请进入 build；也可以直接指定输入文件的绝对路径：

```bash
/mnt/d/TP2_Formal_Approaches/build/compiler /mnt/d/TP2_Formal_Approaches/read.txt
```

这条命令在 WSL 的其他目录里也可以使用。

### 12.4 成功与失败的返回值

程序成功返回 0，输入或文件出错返回 1。在 WSL 中运行程序后，立刻输入下面的命令可以查看上一个命令的返回值：

```bash
echo $?
```

代码先完成整次解析，再输出 AST，所以失败时不会打印一棵不完整的树。当前错误处理遇到第一个错误就停止，不会一次列出所有错误。

## 13. 测试怎么运行

在 WSL 中：

```bash
cd /mnt/d/TP2_Formal_Approaches/build
ctest --output-on-failure
```

也可以直接运行 Python 测试：

```bash
python3 ../tests/test_parser.py ./compiler
```

ctest 是统一运行已注册测试的工具。当前 CMake 注册了一个回归测试入口，这个入口内部运行 13 个 Python 测试方法；其中多个方法还分别检查多种输入。

因此 CTest 显示“1 个测试通过”，Python 显示“13 个测试通过”，两者不冲突。

测试包含合法和非法两类输入：合法输入应返回 0，并生成正确结构的 AST；非法输入应返回 1，并包含预期诊断。

重点检查了运算优先级、左结合、嵌套语句块、六种比较符、for 的八种省略组合、switch 分支、字符串、行号和文件读取。

如果换到其他环境，CMake 在配置时找不到 Python 3，就不会注册这项自动测试；“没有找到测试”不等于测试通过。

## 14. 初学者常见的 C++ 写法

先理解它们在本项目里的作用，不需要现在就掌握 C++ 的全部规则。

| 写法 | 在本项目里怎样理解 |
|---|---|
| #include "tokens.hpp" | 引入其他文件中定义的类型和声明 |
| #pragma once | 防止同一个头文件被重复引入 |
| enum class TokenType | 定义一组有名字的类别 |
| std::string | 保存文字的字符串类型 |
| std::vector<T> | 顺序保存多个 T 类型的元素 |
| auto | 让编译器根据右侧表达式确定变量类型 |
| class / struct | 把有关联的数据和方法组织在一起 |
| public / private | 区分外部可调用的部分和类内部使用的部分 |
| const | 限制通过对应变量或方法修改对象的方式 |
| & | 在本项目的许多参数声明中表示引用，访问已有对象 |
| *input | 通过指针访问它指向的输入流对象 |
| node->print(...) | 调用指针所指向节点的打印方法 |
| nullptr | 指针为空，目前没有指向对象 |
| std::move(...) | 允许把对象的资源转交给另一个对象，常用于避免复制 |
| std::shared_ptr<T> | 管理 T 对象的智能指针 |
| virtual / override | 允许不同种类的节点使用各自的 print 实现 |
| std::optional<Token> | 这里可能有一个 Token，也可能没有 |
| throw | 报告错误，跳出当前正常执行流程 |
| try / catch | 尝试执行一段代码，并捕获其中抛出的错误 |

例如 `std::move(tokens)` 在这里用于把 Token 列表交给 Parser。它不是把光标移到下一个 Token；真正改变解析位置的是 pos_ 的增加。

## 15. 建议的学习顺序与小练习

不要第一次就从头逐行看完整个 parser.cpp。先让每个小例子都能在脑中对应到一棵树。

### 第一步：会运行，认识输出

运行 examples/01_expressions.txt，对照输入找出两个 Assignment 节点。

自问：为什么一个 Assignment 下面先出现加法，另一个先出现乘法？

### 第二步：理解 Token

打开 include/tokens.hpp，然后看 src/scanner.l 和 include/scanner.hpp 中的 add()。

练习：在纸上列出 `int a; a = 5;` 的 Token 类别和原文。

参考顺序：Int、Identifier、Semicolon、Identifier、Assign、Number、Semicolon、End。

### 第三步：理解 Parser 的移动

在 src/parser.cpp 中先看 current()、consume()、assignment() 和 assignment_core()。

练习：假设输入是 `a = 5;`，每次 consume() 后，当前位置会指向哪个 Token？

### 第四步：理解表达式

阅读 factor()，再读 term()，最后读 expr()。

在 build 中，可以不改任何文件，直接通过标准输入尝试：

```bash
printf '%s\n' 'a = 2 + 3 * 4;' | ./compiler -
printf '%s\n' 'a = (2 + 3) * 4;' | ./compiler -
printf '%s\n' 'a = 20 - 5 - 2;' | ./compiler -
```

这里 printf 把一行文本写出来，`|` 把文本交给右侧程序，末尾的 `-` 告诉 compiler 从标准输入读取。

先预测树的根运算符是什么，再运行检查。

### 第五步：理解语句块

阅读 get_root()、statement()、block() 和 if_statement()，然后试一下：

```bash
printf '%s\n' 'if (a > 0) { print(a); } else {}' | ./compiler -
```

自问：Program 和 Block 都能保存多条语句，为什么一个代表整个程序，另一个代表局部语句块？

### 第六步：理解循环和分支

阅读 while_statement()、for_statement()、switch_statement() 和 switch_branch()。

重点自己解释两个问题：for 为什么不能直接复用带分号的 assignment()？switch 分支怎样知道自己已经结束？

### 第七步：主动制造错误

```bash
printf '%s\n' 'a = ;' | ./compiler -
printf '%s\n' 'int a = 1;' | ./compiler -
printf '%s\n' 'print(123);' | ./compiler -
```

先说出哪条规则被违反，再检查报错。能解释错误输入为什么被拒绝，也是理解 Parser 的重要部分。

最后可以试着用自己的话回答：Scanner 和 Parser 分别做什么？AST 为什么是树？为什么本次不会直接输出变量的值？DFS 按什么顺序打印节点？

## 16. 当前项目的能力边界

本项目以你们 TP1 的 GrammerDefinition.txt 为准，支持整套已定义的语法，没有把完整 C++ 的所有写法都加进来。

| 可以写 | 当前不能写 |
|---|---|
| int a; a = 1; | int a = 1; |
| a = 0 - 1; | a = -1; |
| a = a + 1; | a++; 或 a += 1; |
| print(a, "text"); | print(123); 或 print(a + 1); |
| if (a > b) {} | if (a) {} 或 if (a > b && b > 0) {} |
| if (a > b) {} else {} | else 后直接接 if，不加花括号 |
| // 单行注释 | /* 块注释 */ |
| switch(a) { case 0: break; } | while(a > 0) { break; } |

字符串可以为空，必须在一行内；字符串里的 `\n` 保留为原文，不解释成换行。变量名必须以英文字母开头，后续可以有数字和下划线。数字字面量只有整数形式。

当前检查的是词法和语法。未声明变量、重复声明、重复 case、除零等语义问题尚未检查，输入程序也不会被执行。空程序、空语句块在当前语法中是合法的。

读懂这个版本后，再决定是否增加符号表等语义检查，会更容易理解每项扩展解决的具体问题。
