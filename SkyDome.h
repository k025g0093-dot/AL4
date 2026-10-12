#pragma once
#include "KamataEngine.h"

class SkyDome {

public:
	// SkyDome();
	/*~SkyDome();*/
	// 引数にモデルのポインタを追加
	void Initialize(KamataEngine::Model* model);

	void Update();

	void Draw(KamataEngine::Camera* camera);

private:
	KamataEngine::Camera camera_;

	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Model* model_ = nullptr;
};
