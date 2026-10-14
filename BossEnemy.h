#pragma once
#include "AABB.h"
#include "KamataEngine.h"

class Player;
class MapChipField;

struct state {
	int HP_;
};
// ============================================================
// 列挙型
// ============================================================


// 角
enum BossCorner {
	kBossCornerRightBottom,
	kBossCornerLeftBottom,
	kBossCornerRightTop,
	kBossCornerLeftTop,

	kBossCornerNumCorner // 要素数
};

// 行動の種類
enum class BossBehavior {
	kRoot,   // 通常状態
	kAttack, // 攻撃状態
	kUnknown
};

enum class BossAttackPhase { kSave, kCharge, kAfterglow };

class BossEnemy {

public:
	BossEnemy();
	~BossEnemy();

	void Initialize(KamataEngine::Camera* camera, KamataEngine::Model* model);

	void Update();
	void Draw();

	KamataEngine::Vector3 GetWorldPodition();

	AABB GetAABB();
	void OnCollsion(Player* player);

	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	bool isCollisionDisabled_ = false;
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	// 体力の取得（GUI表示用）
	int GetHP() const { return bossState.HP_; }

private:
	// ============================================================
	// 型
	// ============================================================

	// マップとの当たり判定の結果
	struct CollisionMapInfo {
		bool isFloor = false;   // 着地
		bool isCeiling = false; // 天井
		bool isWall = false;    // 壁
		KamataEngine::Vector3 velocity_;
	};

	// ============================================================
	// マップとの当たり判定
	// ============================================================

	void MapCollsion(CollisionMapInfo& info);
	void CheckCollisionRight(CollisionMapInfo& info);
	void CheckCollisionLeft(CollisionMapInfo& info);
	void CheckCollisionTop(CollisionMapInfo& info);
	void CheckCollisionBottom(CollisionMapInfo& info);

	void CheckedMove(const CollisionMapInfo& info); // 判定を取ってから移動
	void IsHitTop(const CollisionMapInfo& info);    // 天井に当たったか
	void IsHitWall(const CollisionMapInfo& info);   // 壁に当たったか
	void IsGrounded(const CollisionMapInfo& info);  // 着地したか

	// マップに当たった面で速度を反射させる
	void BounceOnMap(const CollisionMapInfo& info, float restitution);

	static KamataEngine::Vector3 CornerPositio(const KamataEngine::Vector3& center, BossCorner corner);

	// ============================================================
	// 定数
	// ============================================================

	// 当たり判定のサイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHight = 0.8f;

	// マップに当たったときの減速
	static inline const float kAttenuationLanding = 0.01f; // 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.1f;     // 壁接触時の速度減衰率

	//==================================================
	// private variables
	//==================================================

	state bossState{};

	// ============================================================
	// 変数
	// ============================================================

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	// 見た目
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* modelAttack_ = nullptr;
	KamataEngine::WorldTransform worldTransformAttack_;
	uint32_t textureHandle_ = 0; // 現在未使用

	// 移動
	KamataEngine::Vector3 velocity_ = {};
};
