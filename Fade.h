#pragma once
#include "KamataEngine.h"

class Fade {
public:
	// フェードの状態
	enum class Status {
		None,    // 何もしない
		FadeIn,  // 明るくなる
		FadeOut, // 暗くなる
	};

	void Initialize();
	void Update();
	void Draw();

	// フェード開始の指示
	void Start(Status status, float duration);

	void Stop();

	// フェードが終了したか
	bool IsFinished() const;

private:
	// 描画に必要なもの
	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* sprite_ = nullptr;
	KamataEngine::ObjectColor objectColor_;
	KamataEngine::Vector4 color_ = {0, 0, 0, 0}; // RGBA

	// 制御用
	Status status_ = Status::None;
	float duration_ = 1.0f; // かける時間
	float counter_ = 0.0f;  // 現在の時間
};