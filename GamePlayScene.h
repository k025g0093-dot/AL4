#pragma once
#include "Fade.h"
#include "SceneManager.h"
#include <KamataEngine.h>

#include "MyMath.h"

#include "Player.h"

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




private:

	//内部関数
	void RanGamePlayScene();

	//各種ポインタ関係
	Fade* fade_ = nullptr;
	Player* player_ = nullptr;

	//各モデル読み込み口
	KamataEngine::Model* modelPlayer_ = nullptr;//プレイヤーのモデル
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// ビュープロジェクション（カメラ）
	KamataEngine::Camera camera_;
	// タイトル用のモデルなどが必要な場合はここに追加
	KamataEngine::Model* modelTitle_ = nullptr;
	//オブジェクトごとのcollarコードを取得できるように
	KamataEngine::ObjectColor objectColor_;
	//入力のインスタンス取得
	KamataEngine::Input* input_;
};
