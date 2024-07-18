
/*
 *
 * CS-413 Spring 98
 * shell.l: lexical analyzer for shell
 */

%{
#include <string.h>
#include "y.tab.h"
%}

%%
\n  {
    return NEWLINE;
}

[ \t]  {
    /* Discard spaces and tabs */
}

">"  {
    return GREAT;
}

">>" {
    return APPEND;
}

"<"  {
    return LESS;
}

"&"  {
    return AMPERSAND;
}

"|"  {
    return PIPE;
}

[a-zA-Z0-9./_\-]+  {
    yylval.string_val = strdup(yytext);
    return WORD;
}

. {
    /* Invalid character in input */
    return NOTOKEN;
}

%%
