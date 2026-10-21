#pragma once
#include "AABB.h"
#include "KamataEngine.h"
#include "RandomEngine.h"

class Player;
class MapChipField;

struct state {
	int HP_;
	bool isDeth;
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

// 攻撃の段階
enum class BossAttackPhase { 
	kSave, 
	kCharge,
	kAfterglow };

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
	bool isDeth() const { return bossState.isDeth; }
	// マップのデータを受け取る（壁との当たり判定に使う）
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

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
	// 移動
	// ============================================================

	// 速度の分だけ動いて、壁に当たったら跳ね返る（横と縦を別々に動かす）
	void Move();
	void ChooseRandomDirection();
	// その位置に置いたとき、ボスの四角がブロックに1マスでも重なるか
	bool HitsBlock(const KamataEngine::Vector3& center) const;

	// ============================================================
	// ボスの行動変化
	// ============================================================

	void BehaviorUpdate();

	// ============================================================
	// 攻撃処理
	// ============================================================
	void BehaviorAttackInitialize(); // 攻撃行動の初期化
	void BehaviorAttackUpdate();//攻撃処理の更新処理
	void AttackMapCollision();

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

	//イーズアウトの関数
	float EaseOut(float start, float end, float t);

	// ============================================================
	// 定数
	// ============================================================

	// 当たり判定のサイズ（見た目の大きさもこれに合わせる）
	// Move()は重なるマスを全部調べるので、1.0より大きくしても大丈夫
	static inline const float kWidth = 2.0f;
	static inline const float kHight = 2.0f;

	// 移動
	static inline const float kMoveSpeed = 0.1f;  // 横に進む速さ（1.0未満にすること）
	static inline const float kRestitution = 1.0f; // 壁で跳ね返るときの反発（1.0で減速しない）

	// マップに当たったときの減速
	static inline const float kAttenuationLanding = 0.01f; // 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.1f;     // 壁接触時の速度減衰率

	//移動方向変更用タイマー
	static inline int directionChangeTimer{};
	static inline int kDirectionChangeMinFrames=40;
	static inline int kDirectionChangeMaxFrames=120;

	//行動フェーズ変更用タイマー
	static inline int directionChangePaseTimer{};
	static inline int kDirectionChangeMinPhase = 40;
	static inline int kDirectionChangeMaxPhase = 120;

	static inline int stunTimer {};//実際に減らすタイマー
	static inline int kStunFrames=40;//40Fスタンする
	
	// 突進（1フレームの移動量。1.0以上にすると幅1の壁をすり抜けるので注意）
	static constexpr float kDashStartSpeed = 0.45f; // 出だしの速さ
	static constexpr float kDashMinSpeed = 0.25f;   // 落ち着いた後の速さ
	static constexpr float kDashEaseFrames = 12.0f; // 出だしから最低速度に落ちるまでのフレーム数
	static constexpr int32_t kHitStopFrames = 4;    // 壁に当たったときに止めるフレーム数



	//==================================================
	// private variables
	//==================================================

	state bossState{};
	BossBehavior bossBehaviro{};

		// 攻撃
	struct AttackState {
		BossAttackPhase phase = BossAttackPhase::kSave; // 今の段階
		uint32_t counter = 0;                   // 段階ごとの経過フレーム
		KamataEngine::Vector3 direction{};      // 突進の向き
		int32_t hitStopTimer = 0;               // ヒットストップの残り（現在未使用）
	};
	AttackState attack_;

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
