/*This file contains the language specification for the mission file.
It is used by the "Bison GNU" program to generate a parser for mission files.
This parser can then be used to check the validity of loaded mission files.
*/

%require "3.2"
%language "c++"
%define api.parser.class { MissionParser }

%{
#include <stdio.h>

void yyerror(char *s){
	fprintf(stderr, "error: %s\n", s);
}
%}

%code top{
	static MissionParser::symbol_type yylex(MissionScanner scanner){
		return scanner.get_next_token();
	}

}

%lex-param { MissionScanner &scanner }
%parse-param {MissionScanner &scanner}
%token INT
%token FLOAT
%token THREAD
%token EVENT
%token EOL

%%
full_mission: mission | thread_id_line mission {printf("Found full mission");};

thread_id_line: THREAD "=" INT | "thread=" INT events;

events: EVENT "=" INT | EVENT "=" INT ":" EVENT "=" INT;
//val: int | float;

mission: %empty | mission mission_line EOL;

mission_line: "bla" {printf("Found line!");};

%%

