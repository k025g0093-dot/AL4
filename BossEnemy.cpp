#include "BossEnemy.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include "Player.h"
using namespace KamataEngine;

BossEnemy::BossEnemy() = default;

BossEnemy::~BossEnemy() = default;

void BossEnemy::Initialize(Camera* camera, Model* model) {

	assert(camera);
	assert(model);

	m_camera = camera;
	m_model = model;

	bossState.HP_ = 100;


	worldTransform_.Initialize();
	worldTransform_.translation_.y = 5.0f;
	worldTransform_.translation_.x = 10.0f;
	worldTransform_.scale_.y = 1.0f;

}

void BossEnemy::Update() {

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void BossEnemy::Draw() { m_model->Draw(worldTransform_, *m_camera); }


Vector3 BossEnemy::GetWorldPodition() {

	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB BossEnemy::GetAABB() {

	Vector3 worldPos = GetWorldPodition();

	AABB aabb;

	aabb.min = {worldPos.x - worldTransform_.scale_.x / 2.0f, worldPos.y - worldTransform_.scale_.y / 2.0f, worldPos.z - worldTransform_.scale_.z / 2.0f};
	aabb.max = {worldPos.x + worldTransform_.scale_.x / 2.0f, worldPos.y + worldTransform_.scale_.y / 2.0f, worldPos.z + worldTransform_.scale_.z / 2.0f};

	return aabb;
}

void BossEnemy::OnCollsion( Player* player) {
	if (player->isAttack()||!player->isHit) {

		bossState.HP_ -= 10;
		player->isHit = true;
		return;
	}
}
