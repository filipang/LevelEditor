// Get the current time formatted as string
#include "Log.h"


std::string moony::logGetTimestamp()
{
	time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm time_properties;
	char buffer[32];
	std::string time_stamp_str;

	localtime_s(&time_properties, &current_time);
	asctime_s(buffer, 32, &time_properties);
	time_stamp_str = buffer;
	time_stamp_str.erase(time_stamp_str.rfind('\n'));

	return time_stamp_str;
}

// Get the log stream
std::stringstream& moony::logStream()
{
	static std::stringstream stream;
	return stream;
}

// Get the log stream for error messages
std::stringstream& moony::logError()
{
	logStream() << "\nERROR--" << logGetTimestamp() << "--";
	return logStream();
}

// Get the log stream for debug messages
std::stringstream& moony::logDebug()
{
	logStream() << "\nDEBUG--" << logGetTimestamp() << "--";
	return logStream();
}

// Write the log stream to a file
void moony::logSaveToFile()
{
	std::ofstream file_stream("log.txt");

	if (!file_stream.bad())
		file_stream << logStream().str();
	else
		std::cerr << "Failed to create log file\n";

	file_stream.close();
}