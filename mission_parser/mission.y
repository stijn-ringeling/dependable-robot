/*This file contains the language specification for the mission file.
It is used by the "Bison GNU" program to generate a parser for mission files.
This parser can then be used to check the validity of loaded mission files.
*/

%require "3.2"
%define parse.error verbose

%{
#include <stdio.h>
extern int yylineno;

void yyerror(char *s){
	fprintf(stderr, "error on line %d: %s\n", yylineno, s);
}

%}

%token INT
%token FLOAT
%token THREAD
%token EVENT
%token EOL
%token PARAM
%token EQUAL

%%
full_mission: mission | thread_id_line mission {printf("Found full mission");};

thread_id_line: THREAD EQUAL INT | "thread=" INT events;

events: EVENT EQUAL INT | EVENT EQUAL INT ":" EVENT EQUAL INT;
val: INT | FLOAT;

mission: %empty | mission mission_line;

mission_line: expression | expression ',' mission_line {printf("Found line!");};

expression: PARAM EQUAL val

%%

