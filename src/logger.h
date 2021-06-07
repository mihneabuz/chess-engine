#ifndef _LOGGER_UTIL_H_
#define _LOGGER_UTIL_H_

// initialize log file
void init_logger(std::string file_name);

// write s in log file
void log(std::string s);

// closes log stream
void exit_logger();
#endif
