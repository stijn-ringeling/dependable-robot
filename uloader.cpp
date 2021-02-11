#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "uloader.h"


ULoader::ULoader(char* baseDir) {
	snprintf(this->baseDir, MAX_LEN, baseDir);
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