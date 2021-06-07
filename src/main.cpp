#include <iostream>
#include <string>
#include "logger.h"
#include "interface.h"

#define input std::cin

int main() {
	init_logger("log.txt");
	init_interface();
	std::string cmd;

	std::getline(input, cmd);
	if (cmd != "xboard") {
		log("Not connected to xboard. Aborting...");
		return -1;
	}

	while(std::getline(input, cmd)) {
		log("xboard: " + cmd);

		auto poz = cmd.find(' ', 0);
		if (poz < cmd.size()) execute(cmd.substr(0, poz), cmd);
		else execute(cmd, cmd);
	}

	exit_logger();
	return 0;
}
