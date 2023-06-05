%{
#include <stdio.h>
#include <string.h>

int line_number = 1;
%}

%%
[a-zA-Z][a-zA-Z0-9]*  { printf("%s 单词\n", yytext); }
[0-9]+               { printf("%s 数字\n", yytext); }
[ \t]                { /* ignore whitespace */ }
.                    { printf("%c 符号\n", yytext[0]); }
\n                   { line_number++; }
%%
