[TOC]

## lex描述文件

基本结构：

```c
<定义> (可选)

%%

<规则> (必选)

%%

<代码> (可选)

```

### 定义

定义部分主要包含三部分内容，分别是：

- **C 代码定义**
- **命名正则表达式定义**
- **指令定义**

**C 代码定义**

```c
%{
codes
%}
```

在 `%{` 和 `%}` 之间定义 C 代码，lex 会将这些代码直接拷贝至输出文件中。一般会在这里定义变量，include 语句等。

**命名正则表达式定义**

命名正则表达式定义为一系列命名的正则表达式，用于描述不同的标识符，如：`function`、`let`、`import`，其基本格式如下所示：

```c
name expression
```

命名与正则表达式之间使用空格进行分隔。由于为正则表达式提供了命名，我们可以在其他地方直接引用该命名，引用方式为 `{name}`。如下所示为一个简单的示例：

```c
letter   [a-zA-Z]
digit    [0-9]
punct    [,.:;!?]
nonblank [^ \t]
name     {letter}({letter}│{digit})
```

**指令定义**

指令定义则是通过 lex 提供的一系列以 `%` 开头的指令来修改内置变量的默认值。比如：

- `%array`：将内置的 `yytext` 变量的类型设置为 `char` 数组类型。
- `%pointer`：将内置的 `yytext` 标量的类型设置为 `char` 数组指针类型。
- `%s STATE`：定义一个 `STATE` 状态，`STATE` 可以是任意字符串。lex 默认以 `INITIAL` 作为初始状态。
- `%e size`：定义内置的 NFA 表项的数量。默认值是 1000。
- `%n size`：定义内置的 DFA 表项的数量。默认值是 500。
- `%p size`：定义内置的 move 表项的数量。默认值是 2500。

### 规则

规则部分定义了一系列的 **词法翻译规则**（Lexical Translation Rules），每一条词法翻译规则可以分为两部分：**模式** 和 **动作**。

- 模式：用于描述词法规则的正则表达式
- 动作：模式匹配时要执行的 C 代码

词法翻译规则的基本格式如下所示：

```c
pattern action
// or
pattern {
    action
}
```

 **匹配**

**当词法翻译规则的模式匹配成功时，lex 默认会将匹配的 token 值存储在 `yytext` 变量，将匹配的 token 长度存储在 `yyleng` 变量**。如下所示，我们使用翻译规则来统计代码的字符数、单词数、行数。

```c
%{
    int charcount=0, linecount=0, wordcount=0;
%}
letter [^ \t\n]

%%

{letter}+     {wordcount++; charcount+=yyleng;}
.             charcount++;
\n            {linecount++; charcount++;}
```

在很多场景下，词法分析器必须将识别结果返回给调用者。比如，在编译器中，当识别一个标识符时，必须同时返回 **符号表表项的索引** 和 **token 号**。由于 C 语言的 return 语句只能返回一个值，我们可以通过内置的 `yylval` 变量存储 token 值（即符号表项的索引）和 token 号。如下所示：

```c
{name} { yyval = lookup(yytext); return(NAME); }
```

**状态**

如果词法翻译规则的模式的匹配依赖上下文，那么我们可以有不同的方式来处理。我们可以根据其所依赖上下文的相对位置，分为：**左状态**（Left State） 和 **右状态**（Right State）两种。

**左状态**

左状态的基本格式如下所示：

```c
<STATE>(pattern) { action; BEGIN OTHERSTATE; }
```



其中 `STATE` 为定义部分的状态定义所定义的状态，使用 `%s STATE` 进行定义。

**右状态**

右状态的基本格式如下所示：

```c
pattern/context {action}
```



当匹配到 `pattern` 时，且紧随其后是 `context`，那么才算匹配成功。在这种情况下，`/` 后面的内容仍然位于输入流中，它们可以作为下一个匹配的输入。

### 代码

代码部分用于定义自定义代码，如果希望单独使用 lex，那么我们可以在代码部分里包含 `main` 入口函数。如果代码部分为空，那么 lex 会自动添加默认的 `main` 入口函数，如下所示：

```c
int main()
{
    yylex();
    return 0;
}
```

**注意，`yylex` 包含了我们上述所定义的所有逻辑**。

**案例**

假如我们希望对一段文本内容的空格进行规范化，文本如下：

```
This    text (all of it   )has occasional lapses , in punctuation(sometimes pretty bad) ,( sometimes not so).


(Ha!) Is this: fun? Or what!
```

空格规范化的要求包括：

1. 对于多个连续的空行，只能保留一个
2. 对于多个连续的空格，只能保留一个
3. 标点符号前，删除空格
4. 标点符号后，添加空格
5. 左括号后，右括号前，删除空格
6. 左括号前，右括号后，添加空格
7. 后括号可以紧跟标点符号

**右状态方案**

如下所示，为右边状态方案。我们通过匹配多余的空格，并通过操作 `yytext` 来删除空格。

```c
punct [,.;:!?]   // 匹配所有符号
text [a-zA-Z]    // 匹配所有字母
%%

")"" "+/{punct}         {printf(")");}   // 遇到)+标点，则打印), +表示匹配多个空格
")"/{text}              {printf(") ");}  // 遇到)+字母，则打印)+空格
{text}+" "+/")"         {while (yytext[yyleng-1]==’ ’) yyleng--; ECHO;} // 遇到字母+空格，并紧跟)，则去除所有空格

({punct}|{text}+)/"("   {ECHO; printf(" ");} // 两个括号之间添加空格
"("" "+/{text}          {while (yytext[yyleng-1]==’ ’) yyleng--; ECHO;} // 去除(之后的所有空格

{text}+" "+/{punct}     {while (yytext[yyleng-1]==’ ’) yyleng--; ECHO;} // 去除标点前的所有空格

^" "+                   ; // 匹配以空格开头的所有字符串，忽略，不会进行传递
" "+                    {printf(" ");}  // 多个空格只输出一个空格
.                       {ECHO;} // 
\n/\n\n                 ;  // 保留一个空行
\n                      {ECHO;}

%%
```



**左状态方案**

如下所示，为左状态方案。在右状态方案中，我们需要覆盖每一种情况，很容易遗漏。翻译规则的数量可能会随着我们在空格前后识别的类别数量乘积式增长。通过使用左状态方案可以避免翻译规则乘积式增长。

使用左状态，我们在 lex 内部实现了一个有限自动机，并且指定在不同的状态转移过程中如何处理空格。某些情况下，删除空格；某些情况下，掺入空格。

在左状态方案中，我们定义了 4 种状态，分别对应左括号、右括号、文本、标点符号。不同状态下，匹配相同的内容，其状态转移操作不同。

```c
punct [,.;:!?]
text [a-zA-Z]

%s OPEN  // 定义状态
%s CLOSE
%s TEXT
%s PUNCT

%%

" "+ ;  // 消除所有空格

<INITIAL>"("            {ECHO; BEGIN OPEN;} // (开始则进入OPEN状态
<TEXT>"("               {printf(" "); ECHO; BEGIN OPEN;} // 字母后接(则添加一个空格， 进入OPEN状态
<PUNCT>"("              {printf(" "); ECHO; BEGIN OPEN;} // 标点符号后(则添加空格，同样进入OPEN状态

")"                     {ECHO ; BEGIN CLOSE;} // 匹配到)，进入CLOSE

<INITIAL>{text}+        {ECHO; BEGIN TEXT;}
<OPEN>{text}+           {ECHO; BEGIN TEXT;}
<CLOSE>{text}+          {printf(" "); ECHO; BEGIN TEXT;} // )后的字符添加一个空格
<TEXT>{text}+           {printf(" "); ECHO; BEGIN TEXT;} // 字母后的字母添加一个空格
<PUNCT>{text}+          {printf(" "); ECHO; BEGIN TEXT;} // 标点符号的后的字母添加一个空格

{punct}+                {ECHO; BEGIN PUNCT;} 

\n                      {ECHO; BEGIN INITIAL;}

%%
```

### 使用方式

```shell
$ lex -t count.l > count.c
$ cc -c -o count.o count.c
$ cc -o count count.o -ll
```

通过控制台输入文本，最后输入结束符 Ctrl+D，查看统计结果。



### 任务一

双链DNA分子中，G、C碱基对所占比例越高，其稳定性越强。编写一个lex描述文件，计算指定碱基序列里G、C碱基的比例。

测试输入：`ACGTTGATCGGAATCTTCGT` 预期输出：`0.450`

测试输入：`TTACGGTACCAATGCTAATGCCTA` 预期输出：`0.417`

代码如下：

```c
%{

#include <stdio.h>
int gc_count = 0;
int base_count = 0;

%}


%%
[AaTt]             { base_count++; }
[GgCc]             { gc_count++; base_count++; }
\n                 { printf("%.3f\n", (float)gc_count / base_count); }
.                  ;
%%

```

### 任务二

编写一个lex描述文件，识别出指定文本串里的单词、数字和符号（空格不作处理）。

测试输入：

```c
using namespace std;
int main()
{
    int year = 2022;
    cout << "hello" << endl;
    return 0;
}
```

预期输出：

```c
using 单词
namespace 单词
std 单词
; 符号
int 单词
main 单词
( 符号
) 符号
{ 符号
int 单词
year 单词
= 符号
2022 数字
; 符号
cout 单词
< 符号
< 符号
" 符号
hello 单词
" 符号
< 符号
< 符号
endl 单词
; 符号
return 单词
0 数字
; 符号
} 符号
```

代码如下

```

```

### 任务三

编写一个yacc描述文件，实现具有加法和乘法功能的计算器。

测试输入：`5*7+2` 预期输出：`37`

测试输入：`9+3*6` 预期输出：`27`

lex代码：

```c
%{
#include <stdio.h>
%}

/* 定义 token */
%token NUM
%token PLUS
%token TIMES

/* 定义优先级和结合性 */
%left PLUS
%left TIMES

%%

/* 定义语法规则 */
calc: expr { printf("result: %d\n", $1); }
    ;

expr: expr PLUS term { $$ = $1 + $3; }
    | term          { $$ = $1; }
    ;

term: term TIMES factor { $$ = $1 * $3; }
    | factor            { $$ = $1; }
    ;

factor: NUM { $$ = $1; }
      | '(' expr ')' { $$ = $2; }
      ;

%%

/* 词法分析器 */
int yylex() {
    int c = getchar();

    if (c == '+' || c == '*') {
        return c;
    } else if (isdigit(c)) {
        int val = c - '0';
        while (isdigit(c = getchar())) {
            val = val * 10 + (c - '0');
        }
        ungetc(c, stdin);
        yylval = val;
        return NUM;
    } else if (isspace(c)) {
        return yylex();
    } else if (c == EOF) {
        return 0;
    } else {
        fprintf(stderr, "error: unrecognized character %c\n", c);
        return -1;
    }
}

/* 错误处理函数 */
void yyerror(char const *s) {
    fprintf(stderr, "error: %s\n", s);
}

/* 主程序 */
int main() {
    yyparse();
    return 0;
}

```



## 词法分析

### 任务要求

使用C/C++语言编写PL/0编译程序的词法分析程序。 需要注意的点： 

1. 识别非法字符：如 @ 、 & 和 ! 等； 
2. 识别非法单词：数字开头的数字字母组合； 
3. 标识符和无符号整数的长度不超过8位； 
4. 能自动识别并忽略/*  */及//格式的注释信息； 
5. 词法分析过程中遇到错误后能继续往下识别，并输出错误信息。

**测试说明**

测试输入：

```pascal
const a = 10;
var   b, c;

procedure fun1;
    if a <= 10 then
        begin
            c := b + a;
        end;
begin
    read(b);
    while b # 0 do
        begin
            call fun1;
            write(2 * c);
            read(b);
        end
end.
```

预期输出：

```
(保留字,const)
(标识符,a)
(运算符,=)
(无符号整数,10)
(界符,;)
(保留字,var)
(标识符,b)
(界符,,)
(标识符,c)
(界符,;)
(保留字,procedure)
(标识符,fun1)
(界符,;)
(保留字,if)
(标识符,a)
(运算符,<=)
(无符号整数,10)
(保留字,then)
(保留字,begin)
(标识符,c)
(运算符,:=)
(标识符,b)
(运算符,+)
(标识符,a)
(界符,;)
(保留字,end)
(界符,;)
(保留字,begin)
(保留字,read)
(界符,()
(标识符,b)
(界符,))
(界符,;)
(保留字,while)
(标识符,b)
(运算符,#)
(无符号整数,0)
(保留字,do)
(保留字,begin)
(保留字,call)
(标识符,fun1)
(界符,;)
(保留字,write)
(界符,()
(无符号整数,2)
(运算符,*)
(标识符,c)
(界符,))
(界符,;)
(保留字,read)
(界符,()
(标识符,b)
(界符,))
(界符,;)
(保留字,end)
(保留字,end)
(界符,.)
```

测试输入：

```pascal
const 2a = 123456789;
var   b, c;

//单行注释

/*
* 多行注释
*/

procedure function1;
    if 2a <= 10 then
        begin
            c := b + a;
        end;
begin
    read(b);
    while b @ 0 do
        begin
            call function1;
            write(2 * c);
            read(b);
        end
end.
```

预期输出：

```
(保留字,const)
(非法字符(串),2a,行号:1)
(运算符,=)
(无符号整数越界,123456789,行号:1)
(界符,;)
(保留字,var)
(标识符,b)
(界符,,)
(标识符,c)
(界符,;)
(保留字,procedure)
(标识符长度超长,function1,行号:10)
(界符,;)
(保留字,if)
(非法字符(串),2a,行号:11)
(运算符,<=)
(无符号整数,10)
(保留字,then)
(保留字,begin)
(标识符,c)
(运算符,:=)
(标识符,b)
(运算符,+)
(标识符,a)
(界符,;)
(保留字,end)
(界符,;)
(保留字,begin)
(保留字,read)
(界符,()
(标识符,b)
(界符,))
(界符,;)
(保留字,while)
(标识符,b)
(非法字符(串),@,行号:17)
(无符号整数,0)
(保留字,do)
(保留字,begin)
(保留字,call)
(标识符长度超长,function1,行号:19)
(界符,;)
(保留字,write)
(界符,()
(无符号整数,2)
(运算符,*)
(标识符,c)
(界符,))
(界符,;)
(保留字,read)
(界符,()
(标识符,b)
(界符,))
(界符,;)
(保留字,end)
(保留字,end)
(界符,.)
```

## 语法分析

基于第二章的词法分析程序，使用C/C++语言编写PL/0编译程序的语法分析程序。

测试输入：

```pascal
const a = 10;
var   b, c;

procedure p;
    if a <= 10 then
        begin
            c := b + a;
        end;
begin
    read(b);
    while b # 0 do
        begin
            call p;
            write(2 * c);
            read(b);
        end;
end.
```

预期输出：

```shell
语法正确
```

测试输入：

```pascal
const a := 10;
var   b, c;

procedure p;
    if a <= 10 then
        begin
            c := b + a;
        end;
begin
    read(b);
    while b # 0 do
        begin
            call p;
            write(2 * c);
            read(b);
        end;
end.
```


预期输出：

```shell
(语法错误,行号:1)
```

测试输入：

```pascal
const a = 10;
var   b, c;

//单行注释

/*
* 多行注释
*/

procedure p;
    if a <= 10 then
        begin
            c := b + a
        end;
begin
    read(b);
    while b # 0 do
        begin
            call p;
            write(2 * c);
            read(b);
        end;
end.
```

预期输出：

```shell
(语法错误,行号:13)
```

测试输入：

```pascal
const a = 10;
var   b, c;

//单行注释

/*
* 多行注释
*/

procedure p;
    if a <= 10 then
        begin
            c := b + a;
        end;
begin
    read(b);
    while b # 0
        begin
            call p;
            write(2 * c);
            read(b);
        end;
end.
```


预期输出：

```shell
(语法错误,行号:17)
```

测试输入：

```pascal
const a := 10;
var   b, c d;

//单行注释

/*
* 多行注释
*/

procedure procedure fun1;
    if a <= 10 
        begin
            c = b + a
        end;
begin
    read(b;
    while b # 0 
        begin
            call fun1;
            write 2 * c);
            read(b);
        end;
end.
```

```shell
(语法错误,行号:1)
(语法错误,行号:2)
(语法错误,行号:10)
(语法错误,行号:11)
(语法错误,行号:13)
(语法错误,行号:16)
(语法错误,行号:17)
(语法错误,行号:20)
```

