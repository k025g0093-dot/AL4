#include "RandomEngine.h"

std::random_device RandomEngine::seedGenerator_;
std::mt19937_64 RandomEngine::randomEngine_(RandomEngine::seedGenerator_());

float RandomEngine::GetRandomFloat(float min, float max) {
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(randomEngine_);
}

int32_t RandomEngine::GetRandomInt(int32_t min, int32_t max) {
	std::uniform_int_distribution<int32_t> distribution(min, max);
	return distribution(randomEngine_);
}