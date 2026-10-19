# TP2 — Parser 与 AST

本项目接续 Mouzheng LI 和 Yuefan LIU 的 TP1。语法基准是原样保留的
`GrammerDefinition.txt`，Scanner 的匹配规则来自你们的 TP1。
本文件是运行说明和学习笔记，不是提交用报告。

## 编译与运行

在 WSL Ubuntu 中进入项目：

```bash
cd /mnt/d/TP2_Formal_Approaches
mkdir build && cd build
cmake ..
cmake --build .
./compiler
```

如果 build 已存在，直接 `cd build`，再运行后三条命令。
依赖为 C++17 编译器、CMake、Flex 和 make。当前 WSL 环境已安装这些工具。
默认从 build 中读取 `../read.txt`，输出整个程序的 AST。

也可以指定文件或从标准输入读取：

```bash
./compiler ../examples/01_expressions.txt
./compiler ../examples/tp1_input.txt
printf 'int a; a = 2 + 3 * 4;' | ./compiler -
```

成功返回 0。文件错误、词法错误或语法错误返回 1；诊断写到标准错误。
只有整个输入成功解析后才打印 AST，因此错误输入不会产生半棵树。

## 代码阅读顺序

1. `include/tokens.hpp`：一个 Token 保存类型、原文和行号。End 标记输入结束。
2. `src/scanner.l` 与 `include/scanner.hpp`：Flex 识别单词和符号，
   `add(...)` 把它们存入 vector。每次扫描使用独立对象；不再自带 main。
3. `include/ast.hpp`：定义数字、变量、运算、语句与语句块的节点。
4. `src/parser.cpp`：递归下降解析。current 查看当前 Token；consume
   检查预期类型并前进；不匹配时报告行号、预期内容和实际内容。
5. `src/ast.cpp`：先打印当前节点，再依次递归打印子节点，即前序 DFS。
6. `src/main.cpp`：打开输入文件，连接 Scanner、Parser 和 AST 打印器。

先读懂 examples/01_expressions.txt，再看 read.txt 中的完整例子。

## 表达式是怎样变成树的

Parser 的函数与 EBNF 对应：

```text
expr   -> term   { (+ | -) term }
term   -> factor { (* | /) factor }
factor -> 数字 | 变量 | ( expr )
```

expr 调用 term，term 先处理乘除，因此 `2 + 3 * 4` 的加法右子树是 `3 * 4`。
括号会递归调用 expr，因此 `(2 + 3) * 4` 的乘法左子树是 `2 + 3`。
同一优先级的循环不断把已有树作为左子树，保证减法、除法左结合。
括号和分号参与语法检查，但不会单独保存为 AST 节点。

## 语句解析中的关键点

- Program 和 Block 按顺序保存语句；控制结构的语句块可以递归包含其他语句。
- condition 必须是两个算术表达式和一个比较运算符，不接受单独的变量或数值。
- AssignmentCore 不读取结尾分号。普通 Assignment 加一个分号；for 自己读取
  头部的两个分号。for 的初始化、条件、更新均可省略，输出时显示 `<empty>`。
- switch 先读取零个或多个 case，再读取可选的 default。每个分支读取到下一个
  标签或右花括号为止。break 只能出现在分支末尾，不能用在普通循环或嵌套 if 中。
- 字符串保留原始引号与内容，不解释转义；注释和空白不生成 AST 节点。

## 本版本的语言边界

完全沿用 TP1 的语法：声明不带初始化；print 至少有一个参数，参数只能是变量或
字符串；变量名以字母开头；数值字面量没有符号和小数；控制结构必须使用花括号。
没有增加一元负号、else if、块注释、++、+=、&&、||、! 等扩展。

当前做词法和语法检查，不做符号表、未声明变量、重复声明、重复 case 或除零等
语义检查，也不执行输入程序或生成汇编。按 EBNF，空程序和空语句块均合法。

## 自动测试

Python 3 可用时，CMake 自动注册回归测试：

```bash
ctest --output-on-failure
```

也可以直接运行：

```bash
python3 ../tests/test_parser.py ./compiler
```

测试检查 AST 的精确层次与顺序、优先级、左结合、所有比较符、嵌套结构、for
八种可选项组合、switch 分支、TP1 完整示例、字符串与注释、错误行号和输入文件处理。
非法输入还覆盖语法边界，例如 print(123)、循环内 break 和 default 后再出现 case。

## 后续学习

按 Token 保存、表达式解析、基础语句、控制结构、DFS 与错误处理的顺序逐段理解。
报告应由你们结合实际实现和验证结果自行撰写；这些说明只用于帮助理解代码。
