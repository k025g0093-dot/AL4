#pragma once
#include "AABB.h"
#include "KamataEngine.h"


class BossEnemy
{

public:

	BossEnemy();
	~BossEnemy();

	void Initialize(KamataEngine::Camera* camera, KamataEngine::Model* model);

	void Update();
	void Draw();




private:

	//==================================================
	// private variables
	//==================================================


	// エンジン所有クラスへの参照
	KamataEngine::Camera* m_camera = nullptr;
	KamataEngine::Model* m_model = nullptr;
	KamataEngine::WorldTransform worldTransform_{};

};

