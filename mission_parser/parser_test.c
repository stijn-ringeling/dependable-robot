#include "mission_parser.hpp"

int main(int argc, char** argv)
{

    MissionScanner scanner();
    MissionParser parser(scanner);
    return 0;
}