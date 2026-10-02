#include "SkyDome.h"
#include "KamataEngine.h"
#include "MyMath.h"
#include <cassert>

using namespace KamataEngine;

//SkyDome::SkyDome() { Initialize(); }

void SkyDome::Initialize(KamataEngine::Model* model) {
	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	// モデルの生成
	assert(model);
	model_ = model;
	// 例
	camera_.Initialize();
}

void SkyDome::Update() {
	// スカイドームの大きさを設定
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	// スカイドームの位置を設定（カメラと同じ位置にする）
	// ここでは仮に原点に配置
	worldTransform_.translation_ = {0.0f, 0.0f, 0.0f};
	// アフィン変換行列の合成 (Scale * Rotate * Translate)
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();
	camera_.TransferMatrix();
}

void SkyDome::Draw(KamataEngine::Camera* camera) {

	model_->Draw(worldTransform_, *camera);

}
