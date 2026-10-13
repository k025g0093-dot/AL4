#include "BossEnemy.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <cassert>
#include <numbers>
using namespace KamataEngine;

BossEnemy::BossEnemy() = default;

BossEnemy::~BossEnemy() = default;

void BossEnemy::Initialize(Camera* camera, Model* model) {

	assert(camera);
	assert(model);

	m_camera = camera;
	m_model = model;

	worldTransform_.Initialize();
	worldTransform_.translation_.y = 2.0f;
	worldTransform_.scale_.y = 1.0f;

}

void BossEnemy::Update() {

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void BossEnemy::Draw() { m_model->Draw(worldTransform_, *m_camera); }
