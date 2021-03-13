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
    char outputBuffer[20][100];
    for (int i = 0; i < 20; i++) {
        lines[i] = lineBuffer[i];
        output[i] = outputBuffer[i];
    }
    ULoader::clearOutput(output, 20);
    int lineCount;
    //lines[0] = "vel=%0";
    //lines[1] = "acc=%1";
    float v = 0.1; //driving speed
    float wallD = 0.2; //Distance from wall to track
    float wallThreshold = 0.5; //Threshold distance for wall tracking
    float cornerDistance = 0.1; //Extra distance to drive after tracking wall
    float turnRadius = wallD + 0.235 / 2;//Turn radius
    float turnAngle = 85;
    float params[] = { v, wallD, wallThreshold, cornerDistance, turnRadius, turnAngle };

    ULoader loader = ULoader("./missions");

    loader.loadMission("tunnel.mission", lines, &lineCount);

    //char** output = ULoader::createOutput();
    ULoader::formatMission(lines, output, lineCount, params);
    for (int i = 0; i < lineCount; i++) {
        printf("%s", output[i]);
    }
    //printf(output[0]);// printf("\n");
    //printf(output[1]);// printf("\n");
    //printf("0: %d\n", strlen(output[0]));
    //printf("1: %d\n", strlen(output[1]));
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