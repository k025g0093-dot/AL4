#pragma once
#include "AABB.h"
#include "DeathParticles.h"
#include "KamataEngine.h"

class MapChipField;
class BossEnemy;

// ============================================================
// 列挙型
// ============================================================

// 角
enum Corner {
	kRightBottom,
	kLeftBottom,
	kRightTop,
	kLeftTop,

	kNumCorner // 要素数
};

// 行動の種類
enum class Behavior {
	kRoot,   // 通常状態
	kAttack, // 攻撃状態
	kUnknown
};

// 攻撃の段階
enum class AttackPhase {
	kSave,
	kCharge,
	kAfterglow
};

// ノックバックの段階
enum class KnockbackPhase {
	kStartKnockback,
	kRestructureTheSystem,
	kAfterglow
};

class Player {
public:
	// ============================================================
	// 型
	// ============================================================

	// プレイヤーの向き
	enum class LRDirection { kLeft, kRight };

	// ============================================================
	// 基本の関数
	// ============================================================

	Player();
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	// ============================================================
	// 外から呼ばれるもの
	// ============================================================

	// ボスに当たったとき
	void EnemyOnCollsion(const BossEnemy* bossEnemy);
	// ノックバックを開始する
	void IsKnockBack();
	// マップのデータを受け取る
	void SetMapChipFiled(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// ============================================================
	// ゲッター
	// ============================================================

	KamataEngine::Vector3 GetWorldPodition();
	AABB GetAABB();
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	LRDirection GetLRDirection() const { return lrDirection_; }
	bool IsDead() const { return isDead_; }
	const bool isAttack();

	// ============================================================
	// 外から直接触られている変数
	// ============================================================

	int HP = 5;            // 体力（GamePlaySceneのGUIで表示）
	bool isDead_ = false;  // 死亡フラグ（GamePlaySceneで見ている）
	bool isHit = false;    // ボスに当てたか（BossEnemyで読み書きしている）
	int invincibilityTime;

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
	// 行動ごとの処理
	// ============================================================

	void BehaviorRootInitialize();      // 通常行動の初期化
	void BehaviorAttackInitialize();    // 攻撃行動の初期化
	void BehaviorKnockbackInitialize(); // ノックバックの初期化

	void BehaviorRootUpdate();      // 通常の更新
	void BehaviorAttackUpdate();    // 攻撃の更新
	void BehaviorKnockbackUpdate(); // ノックバックの更新

	void MovePlayer();
	void AttackMapCollision(); // 攻撃中（ノックバック中も含む）のマップ判定と移動

	void ApplyBehaviorRequest(); // 行動の切り替えと初期化
	void UpdateMatrices();       // 行列の計算と転送

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

	static KamataEngine::Vector3 CornerPositio(const KamataEngine::Vector3& center, Corner corner);

	// ============================================================
	// 便利関数
	// ============================================================

	float EaseOut(float start, float end, float t);

	// ============================================================
	// 定数
	// ============================================================

	// 当たり判定のサイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHight = 0.8f;

	// マップに当たったときの減速
	static inline const float kAttenuationLanding = 0.01f; // 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.1f;     // 壁接触時の速度減衰率

	// 突進（1フレームの移動量。1.0以上にすると幅1の壁をすり抜けるので注意）
	static constexpr float kDashStartSpeed = 0.85f; // 出だしの速さ
	static constexpr float kDashMinSpeed = 0.45f;   // 落ち着いた後の速さ
	static constexpr float kDashEaseFrames = 12.0f; // 出だしから最低速度に落ちるまでのフレーム数
	static constexpr int32_t kHitStopFrames = 4;    // 壁に当たったときに止めるフレーム数

	// 跳ね返り
	static constexpr float kBounceSpeed = 0.35f;      // 突進で壁に当たったときの跳ね返りの速さ
	static constexpr float kBounceRestitution = 0.8f; // 漂っている間に壁に当たったときの跳ね返り（1.0で減らない）
	static constexpr float kDriftDamping = 0.99f;     // 漂っているときの1フレームごとの減速（1.0で減速なし）

	// ノックバック
	static constexpr float kKnockbackSpeed = 0.3f;             // ノックバック速度
	static constexpr float kKnockbackUpSpeed = 0.5f;           // ノックバック上方向速度
	static constexpr float kKnockbackAttenuation = 0.05f;      // 減衰率
	static constexpr int32_t kKnockbackDuration = 20;          // ノックバック時間（フレーム）
	static constexpr int32_t kKnockbackAfterglowDuration = 10; // 余韻時間

	// ---------- 現在どこからも使われていない（消してよいか確認用） ----------
	static inline const float kAcceleration = 0.1f;  // 加速度
	static inline const float kAttenuation = 0.1f;   // 減速率
	static inline const float kLimitRunSpeed = 0.3f; // 最大速度
	static inline const float kBlank = 0.0f;         // 天井に当たった際の速度
	static inline const float kTimeTrun = 0.3f;      // 旋回時間＜秒＞
	static constexpr float kAttackSpeed = 0.8f;

	// ============================================================
	// 変数
	// ============================================================

	// エンジンから借りているもの
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

	// 向き
	LRDirection lrDirection_ = LRDirection::kRight;
	float turnFistRotationY_ = 0.0f; // 旋回開始時の角度
	float turnTimer_ = 0.0f;         // 旋回タイマー

	// 行動
	Behavior behavior_ = Behavior::kRoot;
	Behavior behaviorRequest_ = Behavior::kUnknown;

	// 攻撃
	AttackPhase kAttackPhase_ = AttackPhase::kSave;
	uint32_t attackCounter_ = 0; // 攻撃の経過フレーム
	KamataEngine::Vector3 kAttakDirection{};
	int32_t hitStopTimer_ = 0;

	// ノックバック
	bool isKnockBack_ = false;
	KnockbackPhase kKnockBack_ = KnockbackPhase::kStartKnockback;
	uint32_t knockbackCounter_ = 0;
};
