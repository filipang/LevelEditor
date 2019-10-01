#ifndef LOG_H
#define LOG_H

#include <chrono>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

namespace moony
{
	// Get the current time formatted as string
	std::string logGetTimestamp();

	// Get the log stream
	std::stringstream& logStream();

	// Get the log stream for error messages
	std::stringstream& logError();

	// Get the log stream for debug messages
	std::stringstream& logDebug();

	// Write the log stream to a file
	void logSaveToFile();
}


#endif