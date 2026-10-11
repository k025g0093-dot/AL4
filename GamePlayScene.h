#pragma once
#include "Fade.h"
#include "SceneManager.h"
#include <KamataEngine.h>

#include "MyMath.h"

#include "Player.h"
#include "MapChipField.h"//マップの読み込み
#include "stageManager.h"
#include "CameraController.h"
#include "SkyDome.h"

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

	//blockのワールドトランスフォーム
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;



private:

	//block生成
	void GenerateFieldObjects();


	//内部関数
	void RanGamePlayScene();
	void StageUpdate();

	//各種ポインタ関係
	Fade* fade_ = nullptr;
	Player* player_ = nullptr;
	MapChipField* mapChipField_=nullptr;
	stageManager* stageManager_ = nullptr;
	CameraController* cameraController_;
	SkyDome* skyDome_ = nullptr;
	//KamataEngineを使う変数の宣言

	//各モデル読み込み口
	KamataEngine::Model* modelPlayer_ = nullptr;//プレイヤーのモデル
	KamataEngine::Model* modelAttack_ = nullptr; // プレイヤーの攻撃モデル
	KamataEngine::Model* modelMapBlock_ = nullptr; // マップのモデル
	KamataEngine::Model* modelSkyDome_ = nullptr;  // マップのモデル

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


};
