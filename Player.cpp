#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// ============================================================
// 生成・破棄・初期化
// ============================================================

Player::Player() {}

Player::~Player() {}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;

	velocity_ = {0.0f, 0.0f, 0.0f};
	status_.invincibilityTime = 60;

	// 見た目
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.translation_ = position;
	worldTransform_.rotation_ = {0.0f, std::numbers::pi_v<float> * 3.0f / 2.0f, 0.0f};

	UpdateMatrices();
}

// ============================================================
// 毎フレームの更新・描画
// ============================================================

void Player::Update() {
	// 無敵時間を減らす
	if (status_.invincibilityTime > 0) {
		status_.invincibilityTime--;
	}

	// 行動の切り替え
	ApplyBehaviorRequest();

	// 行動ごとの更新
	switch (behavior_.current) {
	case Behavior::kRoot:
		BehaviorRootUpdate();

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			behavior_.request = Behavior::kAttack;
		}
		break;

	case Behavior::kAttack:
		if (knockback_.isActive) {
			BehaviorKnockbackUpdate();
		} else {
			BehaviorAttackUpdate();
			attack_.counter++;
		}
		AttackMapCollision();
		break;

	default:
		break;
	}

	// 余韻が終わったら通常状態へ（跳ね返りの速度は残したまま）
	if (attack_.phase == AttackPhase::kAfterglow && attack_.counter >= 10) {
		attack_.direction = {0.0f, 0.0f, 0.0f};
		worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		attack_.phase = AttackPhase::kSave; // 次の攻撃に備えてリセット
		behavior_.request = Behavior::kRoot;
	}

	UpdateMatrices();
}

void Player::Draw() {
	model_->Draw(worldTransform_, *camera_);
}

// 行動の切り替えリクエストがあれば切り替えて、その行動の初期化をする
void Player::ApplyBehaviorRequest() {
	if (behavior_.request == Behavior::kUnknown) {
		return;
	}

	behavior_.current = behavior_.request;
	switch (behavior_.current) {
	case Behavior::kRoot:
		break;
	case Behavior::kAttack:
		status_.isHit = false;
		attack_.counter = 0;
		break;
	default:
		break;
	}
	behavior_.request = Behavior::kUnknown;
}

// 行列を計算して転送する
void Player::UpdateMatrices() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

// ============================================================
// 行動ごとの処理
// ============================================================

void Player::BehaviorRootInitialize() {}

void Player::BehaviorAttackInitialize() {}

void Player::BehaviorKnockbackInitialize() {}

// ---------- 通常 ----------
void Player::BehaviorRootUpdate() {
	// 移動
	MovePlayer();

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity_ = velocity_;
	MapCollsion(collisionMapInfo);
	BounceOnMap(collisionMapInfo, kBounceRestitution);
	CheckedMove(collisionMapInfo);

	// 旋回
	if (turn_.timer > 0.0f) {
		turn_.timer -= 1.0f / 20.0f;
		if (turn_.timer < 0.0f) {
			turn_.timer = 0.0f;
		}
		float t = 1.0f - turn_.timer;

		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(turn_.direction)];

		worldTransform_.rotation_.y = turn_.firstRotationY + (destinationRotationY - turn_.firstRotationY) * t;
	}
}

void Player::MovePlayer() {
	Input* input = Input::GetInstance();

	// 突進の向き：押しているキーから毎フレーム作り直す（離した軸が残らないように）
	attack_.direction = {0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_D)) {
		attack_.direction.x += 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		attack_.direction.x -= 1.0f;
	}
	if (input->PushKey(DIK_W)) {
		attack_.direction.y += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		attack_.direction.y -= 1.0f;
	}

	// 見た目の向き（左右を押したときだけ旋回）
	if (attack_.direction.x != 0.0f) {
		LRDirection next = attack_.direction.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		if (turn_.direction != next) {
			turn_.direction = next;
			turn_.firstRotationY = worldTransform_.rotation_.y;
			turn_.timer = 1.0f;
		}
	}

	// 無重力：狙う入力では速度を変えず、少しずつ減速しながら漂う
	velocity_.x *= kDriftDamping;
	velocity_.y *= kDriftDamping;
}

// ---------- 攻撃 ----------
void Player::BehaviorAttackUpdate() {
	switch (attack_.phase) {
	case AttackPhase::kSave: { // 溜め
		float t = static_cast<float>(attack_.counter) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
		attack_.counter++;
		if (attack_.counter >= 10) {
			attack_.phase = AttackPhase::kCharge;
			attack_.counter = 0;

			// 突進の向きを確定させる（斜めでも速さが変わらないように長さを1にする）
			float length = std::sqrt(attack_.direction.x * attack_.direction.x + attack_.direction.y * attack_.direction.y);
			if (length > 0.0f) {
				attack_.direction.x /= length;
				attack_.direction.y /= length;
			} else {
				// 何も押していなければ、向いている方へ
				attack_.direction = {turn_.direction == LRDirection::kRight ? 1.0f : -1.0f, 0.0f, 0.0f};
			}
		}
		break;
	}
	case AttackPhase::kCharge: { // 突進
		// 出だしが一番速く、だんだん落ちて最低速度で落ち着く
		float t = std::min(static_cast<float>(attack_.counter) / kDashEaseFrames, 1.0f);
		float ease = 1.0f - (1.0f - t) * (1.0f - t);
		float speed = kDashStartSpeed + (kDashMinSpeed - kDashStartSpeed) * ease;

		velocity_.x = attack_.direction.x * speed;
		velocity_.y = attack_.direction.y * speed;

		// 進んでいる向きに伸ばす（横の成分はz、縦の成分はyを伸ばす）
		float stretch = 1.0f + 0.5f * (speed / kDashStartSpeed);
		float squash = 1.0f / stretch;
		float ax = std::fabs(attack_.direction.x);
		float ay = std::fabs(attack_.direction.y);
		worldTransform_.scale_.z = squash + (stretch - squash) * ax;
		worldTransform_.scale_.y = squash + (stretch - squash) * ay;
		break;
	}
	case AttackPhase::kAfterglow: { // 余韻
		float t = static_cast<float>(attack_.counter) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
		break;
	}
	default:
		break;
	}
}

// 攻撃中（ノックバック中も含む）のマップとの判定と移動
void Player::AttackMapCollision() {
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity_ = velocity_;

	// 1. 衝突判定と押し戻し量の計算
	MapCollsion(collisionMapInfo);
	// 2. 壁衝突後の減衰
	IsHitWall(collisionMapInfo);
	// 3. 地面や天井の判定
	IsHitTop(collisionMapInfo);
	IsGrounded(collisionMapInfo);
	// 4. 最後に座標を確定させる
	CheckedMove(collisionMapInfo);

	bool hitMap = collisionMapInfo.isWall || collisionMapInfo.isCeiling || collisionMapInfo.isFloor;
	if (attack_.phase == AttackPhase::kCharge && hitMap) {
		// 突進中に当たったら余韻へ
		attack_.phase = AttackPhase::kAfterglow;
		attack_.counter = 0; // 余韻を0から数え直す

		// 当たった面で突進の向きを反射して、跳ね返りの速度にする
		Vector3 reflect = attack_.direction;
		if (collisionMapInfo.isWall) {
			reflect.x = -reflect.x;
		}
		if (collisionMapInfo.isCeiling || collisionMapInfo.isFloor) {
			reflect.y = -reflect.y;
		}
		velocity_ = reflect * kBounceSpeed;
	} else {
		// 余韻中に別の壁に当たったときも跳ね返る
		BounceOnMap(collisionMapInfo, kBounceRestitution);
	}
}

// ---------- ノックバック ----------
void Player::BehaviorKnockbackUpdate() {
	switch (knockback_.phase) {
	case KnockbackPhase::kStartKnockback:
		knockback_.counter = 0;
		// 向いている方の逆へ飛ばす
		if (turn_.direction == LRDirection::kRight) {
			velocity_.x = -kKnockbackSpeed;
		} else {
			velocity_.x = kKnockbackSpeed;
		}
		knockback_.phase = KnockbackPhase::kRestructureTheSystem;
		break;

	case KnockbackPhase::kRestructureTheSystem:
		// 減速
		knockback_.counter++;
		velocity_.x *= (1.0f - kKnockbackAttenuation);
		if (knockback_.counter >= kKnockbackDuration) {
			knockback_.phase = KnockbackPhase::kAfterglow;
			knockback_.counter = 0;
		}
		break;

	case KnockbackPhase::kAfterglow:
		// 通常の移動に戻る前の待ち
		knockback_.counter++;
		if (knockback_.counter >= kKnockbackAfterglowDuration) {
			knockback_.isActive = false;
			knockback_.phase = KnockbackPhase::kStartKnockback; // 次のノックバックに備えて初期化
		}
		break;

	default:
		break;
	}
}

// ============================================================
// マップとの当たり判定
// ============================================================

void Player::MapCollsion(CollisionMapInfo& info) {
	CheckCollisionTop(info);
	CheckCollisionBottom(info);
	CheckCollisionRight(info);
	CheckCollisionLeft(info);
}

// ---------- 右 ----------
void Player::CheckCollisionRight(CollisionMapInfo& info) {
	if (info.velocity_.x <= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	MapChipField::IndexSet hitIndex{};

	// 右上（隣のセルは左）
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop] + Vector3(0.1f, 0, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitIndex = indexSet;
	}

	// 右下（隣のセルは左）
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0.1f, 0.0f, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitIndex = indexSet;
	}

	if (hit) {
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float newVelocityX = (rect.left - worldTransform_.translation_.x) - (kWidth / 2.0f) - 0.01f;
		info.velocity_.x = std::min(0.0f, newVelocityX);
		info.isWall = true;
	}
}

// ---------- 左 ----------
void Player::CheckCollisionLeft(CollisionMapInfo& info) {
	if (info.velocity_.x >= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	MapChipField::IndexSet hitIndex{};

	// 左上（隣のセルは右）
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop] + Vector3(-0.1f, 0, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitIndex = indexSet;
	}

	// 左下（隣のセルは右）
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(-0.1f, 0.0f, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitIndex = indexSet;
	}

	if (hit) {
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float newVelocityX = (rect.right - worldTransform_.translation_.x) + (kWidth / 2.0f) + 0.01f;
		info.velocity_.x = std::max(0.0f, newVelocityX);
		info.isWall = true;
	}
}

// ---------- 上 ----------
void Player::CheckCollisionTop(CollisionMapInfo& info) {
	if (info.velocity_.y <= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	Corner hitCorner = kLeftTop; // 当たった角

	// 左上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kLeftTop;
	}

	// 右上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kRightTop;
	}

	if (!hit) {
		return;
	}

	// 移動前と移動後で縦のマスが変わる（天井に入る瞬間）ときだけ押し戻す
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[hitCorner]);
	MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPositio(worldTransform_.translation_, hitCorner));
	if (indexSetNow.yIndex != indexSet.yIndex) {
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.velocity_.y = (rect.bottom - worldTransform_.translation_.y) - (kHight / 2.0f) - 0.01f;
		info.isCeiling = true;
	}
}

// ---------- 下 ----------
void Player::CheckCollisionBottom(CollisionMapInfo& info) {
	if (info.velocity_.y > 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	Corner hitCorner = kLeftBottom; // 当たった角

	// 左下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kLeftBottom;
	}

	// 右下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kRightBottom;
	}

	if (!hit) {
		return;
	}

	// 移動前と移動後で縦のマスが変わる（床に入る瞬間）ときだけ押し戻す
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[hitCorner]);
	MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPositio(worldTransform_.translation_, hitCorner));
	if (indexSetNow.yIndex != indexSet.yIndex) {
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.velocity_.y = (rect.top - worldTransform_.translation_.y) + (kHight / 2.0f) + 0.01f;
		info.isFloor = true;
	}
}

// ---------- 判定結果を使う処理 ----------

// 判定で補正された速度で移動する
void Player::CheckedMove(const CollisionMapInfo& info) { worldTransform_.translation_ += info.velocity_; }

// 左右の壁に当たったら横を、床か天井に当たったら縦を反転する（角なら両方）
void Player::BounceOnMap(const CollisionMapInfo& info, float restitution) {
	if (info.isWall) {
		velocity_.x = -velocity_.x * restitution;
	}
	if (info.isCeiling || info.isFloor) {
		velocity_.y = -velocity_.y * restitution;
	}
}

// 壁に当たったら横の速度を減らす
void Player::IsHitWall(const CollisionMapInfo& info) {
	if (info.isWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// 天井に当たったら縦の速度を0にする
void Player::IsHitTop(const CollisionMapInfo& info) {
	if (info.isCeiling) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0;
	}
}

// 床に当たったら横を減速して縦を0にする
void Player::IsGrounded(const CollisionMapInfo& info) {
	if (info.isFloor) {
		DebugText::GetInstance()->ConsolePrintf("isFloor landing\n");
		velocity_.x *= (1.0f - kAttenuationLanding);
		velocity_.y = 0.0f;
	}
}

// 中心から角の位置を求める
Vector3 Player::CornerPositio(const Vector3& center, Corner corner) {
	float halfW = kWidth / 2.0f;
	float halfH = kHight / 2.0f;

	Vector3 offsetTable[kNumCorner] = {
	    {+halfW, -halfH, 0.0f}, // kRightBottom
	    {-halfW, -halfH, 0.0f}, // kLeftBottom
	    {+halfW, +halfH, 0.0f}, // kRightTop
	    {-halfW, +halfH, 0.0f}  // kLeftTop
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// ============================================================
// 敵との当たり判定
// ============================================================

void Player::EnemyOnCollsion(const BossEnemy* bossEnemy) {
	(void)bossEnemy;

	// 攻撃中はダメージを受けない
	if (isAttack()) {
		return;
	}

	if (status_.invincibilityTime <= 0) {
		status_.HP--;
		status_.invincibilityTime = 60; 
		if (status_.HP <= 0) {
			status_.isDead = true;
		}
	}
	velocity_ += Vector3(0, 0.1f, 0);
}

void Player::IsKnockBack() {
	knockback_.isActive = true;
	knockback_.phase = KnockbackPhase::kStartKnockback; // フェーズをリセット
}

// ============================================================
// ゲッター
// ============================================================

Vector3 Player::GetWorldPodition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPodition();

	AABB aabb;
	aabb.min = {worldPos.x - worldTransform_.scale_.x / 2.0f, worldPos.y - worldTransform_.scale_.y / 2.0f, worldPos.z - worldTransform_.scale_.z / 2.0f};
	aabb.max = {worldPos.x + worldTransform_.scale_.x / 2.0f, worldPos.y + worldTransform_.scale_.y / 2.0f, worldPos.z + worldTransform_.scale_.z / 2.0f};
	return aabb;
}

const bool Player::isAttack() { return behavior_.current == Behavior::kAttack; }

Vector3 Player::GetAimDirection() const {
	// 突進の向きを決めるところ（BehaviorAttackUpdate の溜め）と同じ決め方にする
	float length = std::sqrt(attack_.direction.x * attack_.direction.x + attack_.direction.y * attack_.direction.y);
	if (length > 0.0f) {
		return {attack_.direction.x / length, attack_.direction.y / length, 0.0f};
	}
	return {turn_.direction == LRDirection::kRight ? 1.0f : -1.0f, 0.0f, 0.0f};
}

// ============================================================
// 便利関数
// ============================================================

// 2次のイーズアウト（tは0〜1）
float Player::EaseOut(float start, float end, float t) { return start + (end - start) * (1.0f - (1.0f - t) * (1.0f - t)); }
