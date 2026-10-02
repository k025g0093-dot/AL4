#pragma once
#include "Fade.h"
#include "SceneManager.h"
#include <KamataEngine.h>

#include "MyMath.h"

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

	Fade* fade_ = nullptr;

private:
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// ビュープロジェクション（カメラ）
	KamataEngine::Camera camera_;

	// タイトル用のモデルなどが必要な場合はここに追加
	KamataEngine::Model* modelTitle_ = nullptr;

	// これを追加
	KamataEngine::ObjectColor objectColor_;

	KamataEngine::Input* input_;
};
