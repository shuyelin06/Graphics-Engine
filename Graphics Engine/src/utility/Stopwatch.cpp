#include "Stopwatch.h"

namespace Engine
{
namespace Utility
{
	using namespace std::chrono;

	// Stopwatch Constructor:
	// Initializes a steady_clock which can be uesd
	Stopwatch::Stopwatch()
	{
		Reset();
	}

	// Begin:
	// Starts the stopwatch, by setting start_time to the
	// current time according to chrono's steady_clock
	void Stopwatch::Reset() 
	{
		start_time = steady_clock::now();
	}

	// End:
	// Ends the stopwatch and calculates / returns the difference
	// between the start and end time.
	double Stopwatch::Duration() 
	{
		// Find end time
		time_point<Clock> end_time = steady_clock::now();
		
		// Calculate duration
		duration<double> timespan = duration_cast<duration<double>>(end_time - start_time);
		return timespan.count();
	}
}
}