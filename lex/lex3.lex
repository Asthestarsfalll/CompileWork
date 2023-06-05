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
