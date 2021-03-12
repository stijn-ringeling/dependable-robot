#pragma once

class ULoader {
public:
	//Constructor
	ULoader(char* baseDir);
	//Destructor
	~ULoader();

	//Load mission from file into char** lines.
	//Filepath is "baseDir/location".
	//lineCount is updated by the function to represent number of loaded lines.
	void loadMission(char* location, char** lines, int* lineCount);
	//Allocate and setup memory to be used as an output for formatMission.
	//lines is the number of mission lines to allocate. Default: 20
	//lineLength is the number of characters to allocate per line. Default: 256
	//static char** createOutput(int lines = 20, int lineLength =256);
	//Clear output array so that it is ready for formatting. Should be run before output can be reused.
	//ouput is the pointer returned by createOutput. lines is the number of lines to clear.
	static void clearOutput(char** output, int lines);

	//Free allocated output memory.
	//Should be called when output is no longer needed.
	//static void freeOutput(char** output);

	//Format loaded mission with given parameters
	static void formatMission(char** lines, char** output, int lineCount, float* parameters);

private:
	char baseDir[300];
	const static int MAX_LEN = 100;
	const static int missionLineMax = 20;
};