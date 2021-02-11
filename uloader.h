#pragma once


class ULoader {
public:
	//Constructor
	ULoader(char* baseDir);
	//Destructor
	~ULoader();

	void loadMission(char* location, char** lines, int* lineCount);

private:
	char baseDir[300];
	const static int MAX_LEN = 100;
	const static int missionLineMax = 20;
};