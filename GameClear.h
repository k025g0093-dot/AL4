#pragma once
#include "Fade.h"
#include "SkyDome.h"
#include "SceneManager.h"
#include <KamataEngine.h>

class GameClear : public SceneManager {
public:
	GameClear() = default;
	~GameClear() override;

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

	KamataEngine::Sprite* resultSprite_ = nullptr;
	uint32_t resultTexture_ = 0;
	
	KamataEngine::Sprite* UISprite_ = nullptr;
	uint32_t UITexture_ = 0;

	// 天球（ゲーム中と同じ宇宙の天球を背景に使う）
	KamataEngine::Model* modelSkyDome_ = nullptr;
	SkyDome* skyDome_ = nullptr;

	// これを追加
	KamataEngine::ObjectColor objectColor_;
	KamataEngine::Input* input_;

	// BGM
	uint32_t bgmHandle_ = 0;                   // 読み込んだ音のデータ
	uint32_t bgmVoice_ = 0;                    // 再生中の音（止めるときに使う）
	static inline const float kBgmVolume = 0.5f; // 音量（1.0が元の大きさ）
};
