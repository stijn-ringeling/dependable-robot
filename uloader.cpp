#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "uloader.h"
extern "C" {
#include "mission_parser/mission_lexer.h"
}


ULoader::ULoader(char* baseDir) {
	snprintf(this->baseDir, MAX_LEN, baseDir);
}
ULoader::~ULoader() {

}

void ULoader::loadMission(char* filename, char** lines, int* lineCount) {
	char f[MAX_LEN];
	snprintf(f, MAX_LEN, "%s/%s", this->baseDir, filename);
	printf("Reading mission file: %s\n", f);
	std::ifstream file(f);

	//char** lines;
	std::string line;
	*lineCount = 0;
	while (getline(file, line)) {
		std::cout << "Read line: " << line << "\n";
		snprintf(lines[*lineCount], MAX_LEN, line.c_str());
		//printf("Read line: %s\n", lines[*lineCount]);
		(*lineCount)++;
	}
	if (*lineCount == 0) {
		printf("WARNING! empty mission file\n");
	}
	file.close();
}

/*char** ULoader::createOutput(int lines, int lineLength) {
	int len = sizeof(char*) * lines + sizeof(char) * lines * lineLength;
	char** output;
	output = (char**)malloc(len);
	char* ptr;
	ptr = (char*)(output + lines);
	for (int i = 0; i < lines; i++) {
		output[i] = (char*)(ptr + i * lineLength);
	}

	ULoader::clearOutput(output, lines);
	return output;
}*/

void ULoader::clearOutput(char** output, int lines) {
	for (int i = 0; i < lines; i++) {
		output[i][0] = '\0';

	}
}

/*void ULoader::freeOutput(char** output) {
	free(output);
}*/

void ULoader::formatMission(char** lines, char** output, int lineCount, float* parameters) {
	format_mission(lines, output, lineCount, parameters);
}