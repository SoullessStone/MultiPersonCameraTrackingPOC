#ifndef DO_NOT_DEFINE_LOGGER_MULTIPLE_TIMES

	#define DO_NOT_DEFINE_LOGGER_MULTIPLE_TIMES 

	#include <iostream>

	using namespace std;

	class Logger {
		public:
			static void log(std::string message, int level);
			static void logWithoutNewLine(std::string message, int level);
		private:
			static const bool SHOW_DEBUG = false;
	};

#endif
