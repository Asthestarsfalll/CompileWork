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
