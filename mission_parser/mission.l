/*This file is to be used by flex to generate a lexer for the parser.
It's job is to translate incoming data to tokens that can be used by the parser
*/


%{
#include "mission_parser.hpp"
int yylval;
%}

%option c++
%option yyclass="MissionScanner"
%option noyywrap

%%
"thread" {printf("Found thread");return THREAD;}
"event" {printf("Found event");return EVENT;}
[0-9]+ {yylval = atoi(yytext); printf("Found int %d", yylval); return INT;}
[0-9]?.[0-9]+ {printf("Found float");return FLOAT;}
[\n] {printf("Found newline");return EOL;}
[\t] {printf("Found whitespace");}
. {printf("Mystery character %c\n", *yytext); return *yytext;}

%%
