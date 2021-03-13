/*This file is to be used by flex to generate a lexer for the parser.
It's job is to translate incoming data to tokens that can be used by the parser
*/
%option yylineno
%option caseless

%{
#include "mission_lexer.h"
#include "mission_parser.h"
int yylval, lineCount;
float yyfval;
int curr_col=0, curr_line=0;
char** lines;
float* params;

YY_BUFFER_STATE currBuf;

int yywrap(){
	if(curr_line < lineCount - 1){
		curr_line++;
		printf("\n");
		yy_delete_buffer(currBuf);
		currBuf = yy_scan_string(lines[curr_line]);
		yy_switch_to_buffer(currBuf);
		return 0;
	}
	return 1;
}

%}

%%
%[0-9]+ {yyfval = params[atoi(yytext+1)]; return FLOAT;}
"thread" {return THREAD;}
"event" {return EVENT;}
"vel"|"acc"|"log"|"tr"|"bal"|"head"|"topos"|"edgel"|"edger"|"white"|"irsensor"|"irdist"|"servo"|"pservo"|"vservo"|"label"|"goto"|"event" {return PARAM;}
"=" {return EQUAL;}
[0-9]+ {yylval = atoi(yytext); return INT;}
[0-9]\.[0-9]+ {yyfval = atof(yytext); return FLOAT;}
[\n] {curr_col=0;return EOL;}
[\t] {printf("Found whitespace");}
. {return *yytext;}

%%

void format_mission(char* input_lines[], char** output_lines, int maxLines, float* parameters){
	lines = input_lines;
	lineCount = maxLines;
	params = parameters;
	curr_line = 0;

	currBuf = yy_scan_string(lines[0]);
	yy_switch_to_buffer(currBuf);
	for(int ret = yylex(); ret > 0; ret=yylex()){
		char segment[256];
		switch(ret){
			case INT:
				sprintf(segment, "%d", yylval);
				break;
			case FLOAT:
				sprintf(segment, "%.3f", yyfval);
				break;
			default:
				sprintf(segment, yytext);
				break;
		}
		//printf("%d", curr_line);printf("\n");
		//printf(segment);printf("\n");
		strcat(output_lines[curr_line], segment);
		//printf(output_lines[curr_line]);printf("\n");
		
	}
	yy_delete_buffer(currBuf);
}