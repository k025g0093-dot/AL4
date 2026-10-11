#define NOMINMAX
#include "ShieldEnemy.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#include "Player.h"
#include "GamePlayScene.h"

 using namespace KamataEngine;

// 初期化処理
void ShieldEnemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {

	// アフィン行列に必要な者たち
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, 180.0f, 0.0f};
	worldTransform_.translation_ = {0.0f, 0.0f, 0.0f};

	velocity_ = {0.0f, 0.0f, 0.0f}; // 加速度の初期値

	assert(model); // セッターにモデルを入れる
	model_ = model;

	velocity_ = {-kWalkSpeed, 0, 0};

	// textureHandle_ = textureHandle;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	camera_ = camera;

	// にメーション処理の初期化
	walkTimer_ = 0.0f;
	isDead_ = false;
}

// 更新処理
void ShieldEnemy::Update() {

	if (behaviorRequest_ != ShieldEnemyBehavior::kUnknown) {
		behavior_ = behaviorRequest_;
		switch (behavior_) {
		case ShieldEnemyBehavior::kRoot:
			break;
		case ShieldEnemyBehavior::kDead:
			break;
		case ShieldEnemyBehavior::kUnknown:
			break;
		default:
			break;
		}
		behaviorRequest_ = ShieldEnemyBehavior::kUnknown;
	}

	switch (behavior_) {
	case ShieldEnemyBehavior::kRoot:
		BehaviorRootUpdate();

		if (isDead_) {
			behaviorRequest_ = ShieldEnemyBehavior::kDead;
		}

		break;
	case ShieldEnemyBehavior::kDead:
		BehaviorDeadUpdate();
		break;
	case ShieldEnemyBehavior::kGuard:
		BehaviorGuardUpdate();
	    break;
	case ShieldEnemyBehavior::kUnknown:
		break;
	default:
		break;
	}

#pragma region アフィン行列の作成と行列の更新
	// 仮のスケール・回転・平行移動値を設定
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
#pragma endregion

}

void ShieldEnemy::BehaviorRootUpdate() {
	walkTimer_ += 1.0f / 60.0f;
	float param = std::sin((std::numbers::pi_v<float> * 2.0f) * walkTimer_);
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param +1.0f) / 3.0f;
	worldTransform_.rotation_.y = degree;
	worldTransform_.translation_ += velocity_;
}

void ShieldEnemy::BehaviorDeadUpdate() {

	deadTimer_ += 1.0f / 60.0f;
	float t = deadTimer_ / kDeadAnimationDuration;
	if (t > 1.0f) {
		t = 1.0f;
	}
	worldTransform_.rotation_.x = EaseOut(10.3f, 1.0f, t);
	worldTransform_.rotation_.y = EaseOut(0.01f, 1.0f, t);
	if (t >= 1.0f) {
		isDead_ = true;
	}
}

void ShieldEnemy::Draw() { model_->Draw(worldTransform_, *camera_); }

Vector3 ShieldEnemy::GetWorldPodition() {

	Vector3 worldPos;
	// ワールド座標の平行移動成分
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB ShieldEnemy::GetAABB() {

	Vector3 worldPos = GetWorldPodition();

	AABB aabb;

	aabb.min = {worldPos.x - worldTransform_.scale_.x / 2.0f, worldPos.y - worldTransform_.scale_.y / 2.0f, worldPos.z - worldTransform_.scale_.z / 2.0f};
	aabb.max = {worldPos.x + worldTransform_.scale_.x / 2.0f, worldPos.y + worldTransform_.scale_.y / 2.0f, worldPos.z + worldTransform_.scale_.z / 2.0f};

	return aabb;
}


void ShieldEnemy::OnCollsion(Player* player) {
	if (isCollisionDisabled_) {
		return;
	}
	(void)player;
	if (behavior_ == ShieldEnemyBehavior::kDead) {
		return;
	}

	if (player->isAttack()) {

		if (shieldEnemyLRDirection_ ==LRDirection::kLeft&&
			player->GetLRDirection()==Player::LRDirection::kRight||
		    shieldEnemyLRDirection_ == LRDirection::kRight &&
			player->GetLRDirection() == Player::LRDirection::kLeft) {

			Vector3 effectPos = GetWorldPodition();
			effectPos.x = effectPos.x + 0.5f;
			if (gameScene_) {
				behavior_ = ShieldEnemyBehavior::kGuard;
				player->IsKnockBack();
				//gameScene_->CreateGuardEffect(effectPos);
			}

			return;
		}

		isCollisionDisabled_ = true;
		behaviorRequest_ = ShieldEnemyBehavior::kDead;
		Vector3 effectPos = GetWorldPodition();
		// 既存のGameSceneポインタを使う
		if (gameScene_) {
			//gameScene_->CreateHitEffect(effectPos);
		}
	}
}

void ShieldEnemy::BehaviorGuardUpdate() {
	guardTimer_ += 1.0f / 60.0f;

	// ★ ガード状態：敵も後ろに吹き飛ぶ
	if (shieldEnemyLRDirection_ == LRDirection::kLeft) {
		worldTransform_.translation_.x += 0.1f; // 左向きなら右に吹き飛ぶ
	} else {
		worldTransform_.translation_.x -= 0.1f; // 右向きなら左に吹き飛ぶ
	}

	// ★ ガード時間終了で通常状態に戻す
	if (guardTimer_ >= 0.3f) { // 0.3秒でガード終了
		behaviorRequest_ = ShieldEnemyBehavior::kRoot;
		guardTimer_ = 0.0f;
	}
}

float ShieldEnemy::EaseOut(float start, float end, float t)
{
		return start + (end - start) * (1.0f - (1.0f - t) * (1.0f - t));
}

