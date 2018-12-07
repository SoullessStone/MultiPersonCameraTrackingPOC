#include <Logger.h>
// For Windows...
#include <string>
// End for Windows...

void Logger::log(std::string message, int level)
{
	if (level > 0 || SHOW_DEBUG == true) {
		cout << message << endl;
	}
}

void Logger::logWithoutNewLine(std::string message, int level)
{
	if (level > 0 || SHOW_DEBUG == true) {
		cout << message;
	}
}
