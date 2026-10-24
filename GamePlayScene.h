#pragma once
#include "Fade.h"
#include "SceneManager.h"
#include <KamataEngine.h>

#include "MyMath.h"

#include "Player.h"
#include "BossEnemy.h"

#include "MapChipField.h"//マップの読み込み
#include "stageManager.h"
#include "CameraController.h"
#include "SkyDome.h"
#include "Collision.h"

#include <array>

class GamePlayScene : public SceneManager {
public:
	GamePlayScene() = default;
	~GamePlayScene() override ;

	void InitIalize() override;
	void Updete() override;
	void Draw() override;

	Phese phese_ = Phese::kFadeIn;

	bool finished_ = false;

	bool IsFinished() const { return finished_; }
	bool IsClear() const { return isGameClear; }

	//blockのワールドトランスフォーム
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;



private:

	//block生成
	void GenerateFieldObjects();


	//内部関数
	void RanGamePlayScene();
	void StageUpdate();

	void CheckAllCollisions();

	void BossToPlayerAABB();

	// 体力などの情報をImGuiで表示する（デバッグ用）
	void DrawDebugGui();

	// 体力のUI（画面に出すもの）
	void InitializeHud();
	void UpdateHud();
	void DrawHud();

	// 突進の向きの目印（体力と同じ四角を、プレイヤーの周りの8方向に置き、狙っている方向だけ光らせる）
	void UpdateAimMarker();
	void DrawAimMarker();

	void DeathUpdate();

	//各種ポインタ関係
	Fade* fade_ = nullptr;
	Player* player_ = nullptr;
	BossEnemy* bossEnemy_ = nullptr;
	MapChipField* mapChipField_=nullptr;
	stageManager* stageManager_ = nullptr;
	CameraController* cameraController_;
	SkyDome* skyDome_ = nullptr;
	DeathParticles* deathParticles_ = nullptr;

	//KamataEngineを使う変数の宣言

	//各モデル読み込み口
	KamataEngine::Model* modelPlayer_ = nullptr;//プレイヤーのモデル
	KamataEngine::Model* modelBoss_ = nullptr;//プレイヤーのモデル
	KamataEngine::Model* modelAttack_ = nullptr; // プレイヤーの攻撃モデル
	KamataEngine::Model* modelMapBlock_ = nullptr; // マップのモデル
	KamataEngine::Model* modelSkyDome_ = nullptr;  // マップのモデル
	KamataEngine::Model* modelParticl_ = nullptr;  // プレイヤーの死亡時に出るパーティクル

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// ビュープロジェクション（カメラ）
	KamataEngine::Camera camera_;
	//デバックカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;


	// タイトル用のモデルなどが必要な場合はここに追加
	KamataEngine::Model* modelTitle_ = nullptr;
	//オブジェクトごとのcollarコードを取得できるように
	KamataEngine::ObjectColor objectColor_;
	//入力のインスタンス取得
	KamataEngine::Input* input_;

	//その他のメンバ変数の宣言

	bool isDebugCamera_ = false;
	bool isGameClear=false;

	// ============================================================
	// 体力のUI
	// ============================================================
	// 最大体力（Player.h の Status::HP と、BossEnemy::Initialize の HP_ に合わせる）
	static inline const int kPlayerMaxHP = 5;
	static inline const int kBossMaxHP = 100;

	// ボスの体力バー（画面上の真ん中）
	static inline const float kBossBarWidth = 600.0f;
	static inline const float kBossBarHeight = 18.0f;
	static inline const float kBossBarX = (1280.0f - kBossBarWidth) / 2.0f;
	static inline const float kBossBarY = 24.0f;
	static inline const float kBossBarFrame = 3.0f; // 枠の太さ

	// プレイヤーの体力アイコン（画面左下に四角を並べる）
	static inline const float kHpIconSize = 26.0f;
	static inline const float kHpIconSpace = 8.0f;
	static inline const float kHpIconX = 24.0f;
	static inline const float kHpIconY = 720.0f - 24.0f - kHpIconSize;

	uint32_t hudTexture_ = 0; // 白い1x1の画像に色を付けて使う
	KamataEngine::Sprite* bossHpBack_ = nullptr;
	KamataEngine::Sprite* bossHpFill_ = nullptr;
	std::array<KamataEngine::Sprite*, kPlayerMaxHP> playerHpIcons_{};

	// 突進の向きの目印（0番が右、そこから反時計回りに45度ずつ）
	static inline const int kAimDotCount = 8;          // 8方向
	static inline const float kAimDotRadius = 1.5f;    // プレイヤーの中心からの距離（ワールド）
	static inline const float kAimDotSize = 12.0f;     // 狙っていない方向の四角の大きさ（ピクセル）
	static inline const float kAimDotLitSize = 18.0f;  // 狙っている方向の四角の大きさ（ピクセル）
	std::array<KamataEngine::Sprite*, kAimDotCount> aimDots_{};
	bool isAimVisible_ = false;


	// BGM
	uint32_t bgmHandle_ = 0;                   // 読み込んだ音のデータ
	uint32_t bgmVoice_ = 0;                    // 再生中の音（止めるときに使う）
	static inline const float kBgmVolume = 0.5f; // 音量（1.0が元の大きさ）
};
