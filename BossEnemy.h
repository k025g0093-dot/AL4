#pragma once
#include "AABB.h"
#include "KamataEngine.h"

class Player;

struct state {
	int HP_;
};

class BossEnemy
{

public:

	BossEnemy();
	~BossEnemy();

	void Initialize(KamataEngine::Camera* camera, KamataEngine::Model* model);

	void Update();
	void Draw();

	KamataEngine::Vector3 GetWorldPodition();

	AABB GetAABB();
	void OnCollsion(const Player* player);

	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	bool isCollisionDisabled_ = false;
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	// 体力の取得（GUI表示用）
	int GetHP() const { return bossState.HP_; }

private:

	//==================================================
	// private variables
	//==================================================

 state bossState{};

	// エンジン所有クラスへの参照
	KamataEngine::Camera* m_camera = nullptr;
	KamataEngine::Model* m_model = nullptr;
	KamataEngine::WorldTransform worldTransform_{};

};

