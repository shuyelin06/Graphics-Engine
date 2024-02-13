#pragma once

#include <chrono>

namespace Engine
{
namespace Utility
{
	// Shortened type name for the steady_clock
	typedef std::chrono::steady_clock Clock;

	// Contains timing functions which can be useful 
	// for CPU method benchmarking, and FPS limiting
	class Stopwatch
	{
	private:
		std::chrono::time_point<Clock> start_time;

	public:
		Stopwatch();

		void Reset();         // Begin / Reset Stopwatch
		double Duration();	  // Time Elapsed Since Start
	};
}
}