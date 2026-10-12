#pragma once
#include <KamataEngine.h>

struct AABB {
	KamataEngine::Vector3 min; // 左下奥の点
	KamataEngine::Vector3 max; // 右上前の手前の点
};