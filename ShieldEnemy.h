#pragma once
#include "KamataEngine.h"
#include "AABB.h"
#include "MyMath.h"
class Player;
class GamePlayScene;
enum class ShieldEnemyBehavior {
	kRoot,   //(通常状態を表す)
	kDead, //(死亡状態を表す)
	kGuard,
	kUnknown
};

class ShieldEnemy {

public:

	// 敵のガード方向を決めるイーナム
	enum class LRDirection { kLeft, kRight };
	// 初期の向き
	LRDirection shieldEnemyLRDirection_ = LRDirection::kLeft;

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	KamataEngine::Vector3 GetWorldPodition();
	AABB GetAABB();
	void OnCollsion(Player* player);

	//ですフラグ
	bool isDead_ = false;
	bool IsDead() const { return isDead_; };

	// 現在のビヘイビアを表す変数
	ShieldEnemyBehavior behavior_ = ShieldEnemyBehavior::kRoot;

	ShieldEnemyBehavior behaviorRequest_ = ShieldEnemyBehavior ::kUnknown;

	bool isCollisionDisabled_ = false;
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }
	
	void SetGameScene(GamePlayScene* gameScene) { gameScene_ = gameScene; }


private: // プライベート関数群とかのその他

	    GamePlayScene* gameScene_ = nullptr;


	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// 3Dモデルで必要なモデルの呼び出し
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	// 敵の移動速度
	KamataEngine::Vector3 velocity_ = {};

#pragma region 敵の移動に関するもの
	// 加速度を入れるもの
	static inline const float kWalkSpeed = 0.01f;
	//アニメーション項目
	//最初の角度
	static inline const float kWalkMotionAngleStart = 1.0f;
	//最後の角度
	static inline const float kWalkMotionAngleEnd = 1.5f;
	//アニメーションの周期時間
	static inline const float kWalkMotionTimer = 1;

	//経過時間
	float walkTimer_ = 0.0f;
#pragma endregion

static inline const float kDeadAnimationDuration = 0.5f;

	float deadTimer_ = 0.0f;


private://プライベート関数

	//ビヘイビアによる更新
	void BehaviorRootUpdate();

	//死亡時の更新
	void BehaviorDeadUpdate();

	void BehaviorGuardUpdate();

	float EaseOut(float start, float end, float t);
	float guardTimer_ = 0.0f;
};
