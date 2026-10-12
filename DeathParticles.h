#pragma once
#define _USE_MATH_DEFINES
#include "KamataEngine.h"
#include <array>
#include <cmath>
#include <numbers> // これを追加

class DeathParticles {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update(); // UpdataからUpdateに直しておくと後が楽です
	void Draw();

	bool isFinished_ = false;
	bool IsFinished() const { return isFinished_; }
	float counter_ = 0.0f;

private:
	static inline const uint32_t kNumParticles = 8;
	std::array<KamataEngine::WorldTransform, kNumParticles> worldTransforms_;
	// 各パーティクルの速度を保存する配列を追加
	std::array<KamataEngine::Vector3, kNumParticles> velocities_;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	static inline const float kDuration = 2.0f;
	// 定数群
	static inline const float kSpeed = 0.05f;
	// 360度をパーティクル数で割った角度（ラジアン）
	static inline const float kAngleUnit = 2.0f * std::numbers::pi_v<float> / kNumParticles;

	//オブジェクトの色
	KamataEngine::ObjectColor objectColor_;
	//色
	KamataEngine::Vector4 color;


}; 
