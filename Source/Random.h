//
//  Random.h
//  GEngine
//
//  Created by Clark Kromenaker on 8/2/19.
//
#pragma once
#include <chrono>
#include <functional>
#include <random>

namespace Random
{
	// A default random generator.
	// Seeded by the current time to ensure different results on different runs of the program.
	std::default_random_engine generator { (unsigned int)std::chrono::system_clock::now().time_since_epoch().count() };
	
	inline float Range(float minInclusive, float maxInclusive)
	{
		std::uniform_real_distribution<float> distribution(minInclusive, maxInclusive);
		return distribution(generator);
	}
	
	inline int Range(int minInclusive, int maxExclusive)
	{
		std::uniform_int_distribution<int> distribution(minInclusive, maxExclusive - 1);
		return distribution(generator);
	}
}
