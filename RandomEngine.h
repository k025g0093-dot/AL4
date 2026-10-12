#pragma once
#include <random>
class RandomEngine {

public:
	static float GetRandomFloat(float min, float max);
	static int32_t GetRandomInt(int32_t min, int32_t max);

private:
	static std::random_device seedGenerator_;
	static std::mt19937_64 randomEngine_;
};