#include <Clock.h>
// For Windows...
#include <string>
// End for Windows...

// Starts a measurement
void Clock::tic()
{
	start = std::chrono::system_clock::now();
}

// Stops the measurement, logs the result and restarts another
void Clock::toc(std::string message)
{
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end - start;
	Logger::log(message + std::to_string(diff.count()) + "s", 1);
	start = end;
}
