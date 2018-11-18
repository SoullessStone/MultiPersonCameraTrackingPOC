#include <iostream>
#include <Logger.h>
#include <chrono>

using namespace std;

class Clock {
	public:
		void tic();
		void toc(std::string message);
	private:
		std::chrono::time_point<std::chrono::system_clock> start;
};
