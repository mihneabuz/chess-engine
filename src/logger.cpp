#include <fstream>
#include <string>
#include "logger.h"

static std::ofstream log_stream;

void init_logger(std::string file_name) {
	log_stream = std::ofstream(file_name);
}

void log(std::string s) {
	log_stream << s << std::endl;
}

void exit_logger() {
	log_stream.close();
}
