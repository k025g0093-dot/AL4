#define NOMINMAX
#include "DeathParticles.h"
#include "MyMath.h"
#include <algorithm>
#include <cassert>
#include <numbers>
using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model; // ★これを忘れると Draw で落ちる
	camera_ = camera;
	counter_ = 0;
	objectColor_.Initialize();
	color = {1, 1, 1, 1};

	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
	}
}

void DeathParticles::Update() {
	if (isFinished_) {
		return;
	}


	counter_ += 1.0f / 60.0f;

	if (counter_ >= kDuration) {

		counter_ = kDuration;
		isFinished_ = true;
	}

	color.w = std::clamp(1.0f - (counter_ / kDuration), 0.0f, 1.0f);

	objectColor_.SetColor(color);

	for (uint32_t i = 0; i < kNumParticles; i++) {

		Vector3 velocity = {kSpeed, 0, 0};
		float angle = kAngleUnit * i;
		Matrix4x4 matrixRotation = MakeRotateZ(angle);
		velocity = Transform(velocity, matrixRotation);

		worldTransforms_[i].translation_ += velocity;
	}

	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

		worldTransform.TransferMatrix();
	}
}

void DeathParticles::Draw() {

	if (isFinished_) {return;}

	for (WorldTransform& worldTransform : worldTransforms_) {

		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}
