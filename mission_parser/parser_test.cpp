#include "mission_parser.h"
#include "mission_lexer.h"
#include "../uloader.h"

#include <string.h>
#include <stdio.h>



int main(int argc, char** argv)
{
    char* lines[20];
    char lineBuffer[20][100];
    char* output[20];
    char outputBuffer[20][256];
    for (int i = 0; i < 20; i++) {
        lines[i] = lineBuffer[i];
        output[i] = outputBuffer[i];
    }
    ULoader::clearOutput(output, 20);
    int lineCount;
    //lines[0] = "vel=%0";
    //lines[1] = "acc=%1";
    float params[] = { 0.5, 1.5, 2.5 };

    ULoader loader = ULoader("./missions");

    loader.loadMission("parser_test.mission", lines, &lineCount);

    //char** output = ULoader::createOutput();
    ULoader::formatMission(lines, output, lineCount, params);
    printf(output[0]);// printf("\n");
    printf(output[1]);// printf("\n");
    //ULoader::freeOutput(output);
    /*int len = sizeof(char*) * 20 + sizeof(char) * 20 * 256;
    char** output;
    output = (char**)malloc(len);
    //output = (char*)malloc(sizeof(char) * 20 * 256);
    int i;
    char* ptr;
    ptr = (char*)(output + 20);
    for(i = 0; i < 20; i++) {
        output[i] = (char*) (ptr + i*256);
        output[i][0] = '\0';
    }
    //memset(output, '\0', sizeof(output[0][0])*20 * 256);
    float params[] = { 15.0, 7.5};
    lines[0] = "vel=%1";
    lines[1] = "acc=1";

    format_mission(lines, output, 2, params);
    printf("\nReceived output\n");
    printf(output[0]); printf("\n");
    printf(output[1]);
    free(output);*/
    return 0;
}