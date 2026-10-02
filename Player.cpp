#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;
Player::Player() {}
bool isHit = false;
void Player::Initialize(
    KamataEngine::Model* model, KamataEngine::Model* modelAttack, KamataEngine::Camera* camera, const KamataEngine::Vector3& position

) {

	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, 90.0f, 0.0f};
	worldTransform_.translation_ = {0.0f, 0.0f, 0.0f};

	velocity_ = {0.0f, 0.0f, 0.0f};

	assert(model);
	assert(modelAttack);
	modelAttack_ = modelAttack;
	model_ = model;

	worldTransform_.Initialize();
	worldTransformAttack_.Initialize();

	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> * 3.0f / 2.0f;
	camera_ = camera;
}

void Player::Updata() {

#pragma region ビヘイビアの切り替え処理と初期化
	if (behaviorRequest_ != Behavior::kUnknown) {

		behavior_ = behaviorRequest_;
		switch (behavior_) {
		case Behavior::kRoot:
			break;
		case Behavior::kAttack:
			attackCounter_ = 0;
			break;
		default:
			break;
		}
		behaviorRequest_ = Behavior::kUnknown;
	}
#pragma endregion

#pragma region ビヘイビアによる各更新処理

	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			behaviorRequest_ = Behavior::kAttack;
		}

		break;

	case Behavior::kAttack:

		// ★ ノックバック中と通常攻撃で処理を分ける
		if (isKnockBack_) {
			BehaviorKnockbackUpdate();
		} else {
			BehaviorAttackUpdate();
			attackCounter_++;
		}

		// ★ ノックバック中も攻撃中も共通の衝突判定と移動処理を実行

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

		// ★ ノックバック終了判定
		if (!isKnockBack_ && attackCounter_ >= 10) {
			velocity_.x = 0.0f; // 攻撃終了後に横方向の速度をリセット
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
			kAttackPhase_ = AttackPhase::kSave; // 次の攻撃に備えてリセット
			behaviorRequest_ = Behavior::kRoot;
		}

		break;

	//default:
	//	break;
	}
#pragma endregion

#pragma region アフィン行列の作成と行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();

	if (behavior_ == Behavior::kAttack) {
		worldTransformAttack_.matWorld_ = MakeAffineMatrix(worldTransformAttack_.scale_, worldTransformAttack_.rotation_, worldTransformAttack_.translation_);
		worldTransformAttack_.TransferMatrix();
	}
#pragma endregion
}

void Player::Draw() {
	model_->Draw(worldTransform_, *camera_);
	if (behavior_ == Behavior::kAttack) {
		modelAttack_->Draw(worldTransformAttack_, *camera_);
	}
}

Player::~Player() {}

#pragma region ビヘイビアに関する処理（リセットと更新）

void Player::BehaviorRootInitialize() {}

void Player::BehaviorAttackInitialize() {}

void Player::BehaviorKnockbackInitialize() {};

void Player::BehaviorRootUpdate() {
#pragma region プレイヤーの移動処理

	MovePlayer();

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity_ = velocity_;
	// 1. 衝突判定と押し戻し量の計算
	MapCollsion(collisionMapInfo);
	// 2. 壁衝突後の減衰（MapCollsion の後に呼ぶことで isWall フラグが正しく参照される）
	IsHitWall(collisionMapInfo);
	// 3. 状態判定（座標を動かす前に、計算された移動量を使って判定を済ませる）
	IsHitTop(collisionMapInfo);
	IsGrounded(collisionMapInfo);
	// 4. 最後に一回だけ座標を確定させる
	CheckedMove(collisionMapInfo);
#pragma endregion

#pragma region プレイヤーの旋回処理

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 20.0f;
		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}
		float t = 1.0f - turnTimer_;

		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		worldTransform_.rotation_.y = turnFistRotationY_ + (destinationRotationY - turnFistRotationY_) * t;
	}

#pragma endregion
}

void Player::BehaviorAttackUpdate() {
#pragma region プレイヤーの攻撃処理

	if (lrDirection_ == LRDirection::kRight) {
		velocity_.x += kAcceleration;
	} else if (lrDirection_ == LRDirection::kLeft) {
		velocity_.x += -kAcceleration;
	}
	velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
	velocity_.y = 0;

	Vector3 offset;
	if (lrDirection_ == LRDirection::kRight) {
		offset = Vector3(0.8f, 0.0f, 0.0f); // 右方向に0.8ユニット前
	} else if (lrDirection_ == LRDirection::kLeft) {
		offset = Vector3(-0.8f, 0.0f, 0.0f); // 左方向に0.8ユニット前
	}

	worldTransformAttack_.translation_ = worldTransform_.translation_ + offset;
	worldTransformAttack_.rotation_ = worldTransform_.rotation_;

	switch (kAttackPhase_) {
	case AttackPhase::kSave: // 溜め
	{
		float t = static_cast<float>(attackCounter_) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
		if (attackCounter_ >= 10) {
			kAttackPhase_ = AttackPhase::kCharge;
			attackCounter_ = 0;
		}
		break;
	}
	case AttackPhase::kCharge: // 突進動作
	{

		if (lrDirection_ == LRDirection::kRight) {
			velocity_.x = kAttackSpeed;
		} else if (lrDirection_ == LRDirection::kLeft) {
			velocity_.x = -kAttackSpeed;
		}

		float t = static_cast<float>(attackCounter_) / 10.0f;
		worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EaseOut(1.6f, 0.7f, t);
		if (attackCounter_ >= 10) {
			kAttackPhase_ = AttackPhase::kAfterglow; // ★ kAfterglowに変更
			attackCounter_ = 0;
		}
		break;
	}
	case AttackPhase::kAfterglow: // 余韻動作
	{
		float t = static_cast<float>(attackCounter_) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
		break;
	}
	default:
		break;
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity_ = velocity_;
	// 1. 衝突判定と押し戻し量の計算
	MapCollsion(collisionMapInfo);

	// 【★追加】壁衝突後の減衰
	IsHitWall(collisionMapInfo);

	// 【★追加】攻撃中も地面や天井の判定を行い、大元の速度（velocity_）をリセットする
	IsHitTop(collisionMapInfo);
	IsGrounded(collisionMapInfo);

	// 最後に一回だけ座標を確定させる
	CheckedMove(collisionMapInfo);
#pragma endregion
}

void Player::BehaviorKnockbackUpdate() {

	switch (kKnockBack_) {
	case KnockbackPhase::kStartKnockback:
		// ノックバック開始時の初期化
		knockbackCounter_ = 0;
		// 敵がいる方向の逆方向にノックバック
		if (lrDirection_ == LRDirection::kRight) {
			velocity_.x = -kKnockbackSpeed;
		} else {
			velocity_.x = kKnockbackSpeed;
		}
		// velocity_.y = kKnockbackUpSpeed;
		kKnockBack_ = KnockbackPhase::kRestructureTheSystem;
		break;

	case KnockbackPhase::kRestructureTheSystem:
		// ノックバック中の減衰処理
		knockbackCounter_++;
		velocity_.x *= (1.0f - kKnockbackAttenuation);

		if (knockbackCounter_ >= kKnockbackDuration) {
			kKnockBack_ = KnockbackPhase::kAfterglow;
			knockbackCounter_ = 0;
		}
		break;

	case KnockbackPhase::kAfterglow:
		// 余韻処理（通常の移動に戻る前の調整）
		knockbackCounter_++;
		if (knockbackCounter_ >= kKnockbackAfterglowDuration) {
			isKnockBack_ = false;
			kKnockBack_ = KnockbackPhase::kStartKnockback; // 次のノックバックに備えて初期化
		}
		break;

	default:
		break;
	}
}

#pragma endregion

void Player::MovePlayer() {

#pragma region プレイヤーの移動処理
	if (onGround_) {

		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {

			Vector3 acceleration = {};

			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;

					turnFistRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 1.0f;
				}

				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}

				acceleration.x += kAcceleration;
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {

				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;

					turnFistRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 1.0f;
				}

				if (velocity_.x > 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}

				acceleration.x -= kAcceleration;
			}
			velocity_.x = acceleration.x;

			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}

		if (Input::GetInstance()->PushKey(DIK_UP)) {
			velocity_ += Vector3(0, kJumpAcceleration, 0);
		}
	} else {
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
#pragma endregion
}

void Player::CheckedMove(const CollisionMapInfo& info) { worldTransform_.translation_ += info.velocity_; }

#pragma region playerとマップチップの当たり判定

void Player::MapCollsion(CollisionMapInfo& info) {
	CheckCollisionTop(info);
	CheckCollisionBottom(info);
	CheckCollisionRight(info);
	CheckCollisionLeft(info);
}
#pragma endregion

#pragma region 横方向の当たり判定

void Player::CheckCollisionRight(CollisionMapInfo& info) {
	if (info.velocity_.x > 0.0f) {
		std::array<Vector3, kNumCorner> positionsNew;
		for (uint32_t i = 0; i < positionsNew.size(); ++i) {
			positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
		}

		MapChipType mapChipType;
		MapChipType mapChipTypeNext;
		MapChipField::IndexSet indexSet;
		bool hit = false;
		MapChipField::IndexSet hitIndex{};

		// 右上点の判定（隣接セル：左方向）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop] + Vector3(0.1f, 0, 0));
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
			hitIndex = indexSet;
		}

		// 右下点の判定（隣接セル：左方向）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0.1f, 0.5f, 0));
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
}

void Player::CheckCollisionLeft(CollisionMapInfo& info) {
	if (info.velocity_.x < 0.0f) {
		std::array<Vector3, kNumCorner> positionsNew;
		for (uint32_t i = 0; i < positionsNew.size(); ++i) {
			positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
		}

		MapChipType mapChipType;
		MapChipType mapChipTypeNext;
		MapChipField::IndexSet indexSet;
		bool hit = false;
		MapChipField::IndexSet hitIndex{};

		// 左上点の判定（隣接セル：右方向）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop] + Vector3(-0.1f, 0, 0));
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
			hitIndex = indexSet;
		}

		// 左下点の判定（隣接セル：右方向）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(-0.1f, 0.5f, 0));
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
}

void Player::IsHitWall(const CollisionMapInfo& info) {

	if (info.isWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

#pragma endregion

#pragma region playerとマップチップの上方向判定

void Player::CheckCollisionTop(CollisionMapInfo& info) {
	if (info.velocity_.y > 0.0f) {
		std::array<Vector3, kNumCorner> positionsNew;
		for (uint32_t i = 0; i < positionsNew.size(); ++i) {
			positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
		}

		MapChipType mapChipType;
		MapChipType mapChipTypeNext;
		MapChipField::IndexSet indexSet;
		bool hit = false;
		Corner hitCorner = kLeftTop; // ★ヒットした角を保持する変数を追加

		// 左上点の判定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
			hitCorner = kLeftTop;
		}

		// 右上点の判定
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
			hitCorner = kRightTop;
		}

		if (hit) {
			// ヒットした角の移動後のインデックスを正確に取得
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[hitCorner]);

			// ★移動前（現在）のインデックスを取得
			MapChipField::IndexSet indexSetNow;
			indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPositio(worldTransform_.translation_, hitCorner));

			// ★移動前と移動後のyインデックスが異なる（天井を突き抜ける瞬間）なら判定する
			if (indexSetNow.yIndex != indexSet.yIndex) {
				MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
				float newVelocityY = (rect.bottom - worldTransform_.translation_.y) - (kHight / 2.0f) - 0.01f;

				// ★std::min を外して、計算した押し戻し量をそのまま代入
				info.velocity_.y = newVelocityY;
				info.isCeiling = true;
			}
		}
	}
}

// 天井に当たったときに速度を０にする処理
void Player::IsHitTop(const CollisionMapInfo& info) {

	if (info.isCeiling) { // 修正: isFloor → isCeiling
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0;
	}
}

#pragma endregion

#pragma region playerとマップチップの地面判定

void Player::CheckCollisionBottom(CollisionMapInfo& info) {
	if (info.velocity_.y > 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	Corner hitCorner = kLeftBottom; // ★ヒットした角を保持する変数を追加

	// 左下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kLeftBottom; // ★左下でヒット
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = kRightBottom; // ★右下でヒット
	}

	if (hit) {
		// ヒットした角の移動後のインデックスを正確に取得
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[hitCorner]);

		// 移動前（現在）のインデックスを取得
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPositio(worldTransform_.translation_, hitCorner));

		// 移動前と移動後のyインデックスが異なる（床を突き抜ける瞬間）なら判定する
		if (indexSetNow.yIndex != indexSet.yIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			float newVelocityY = (rect.top - worldTransform_.translation_.y) + (kHight / 2.0f) + 0.01f;

			// 【★修正】std::min を外して、計算した押し戻し量をそのまま入れる
			info.velocity_.y = newVelocityY;

			// 地面に当たったことを記録する
			info.isFloor = true;
		}
	}
}

void Player::IsGrounded(const CollisionMapInfo& info) {

	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {

			std::array<Vector3, kNumCorner> positionsNew;
			for (uint32_t i = 0; i < positionsNew.size(); ++i) {
				positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<Corner>(i));
			}

			MapChipType maoChipType;
			bool hit = false;

			MapChipField::IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0, -0.05f, 0));
			maoChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (maoChipType == MapChipType::kBlock) {
				hit = true;
			}

			indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0, -0.05f, 0));
			maoChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (maoChipType == MapChipType::kBlock) {
				hit = true;
			}

			if (!hit) {
				onGround_ = false;
			}
		}

	} else {

		if (info.isFloor) { // 修正: isRoof → isFloor
			DebugText::GetInstance()->ConsolePrintf("isFloor landing\n");
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;
		}
	}
}

#pragma endregion

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

void Player::EnemyOnCollsion(const Enemy* enemy) {
	if (isAttack()) {
		return;
	}
	(void)enemy;
	isDead_ = true;
	velocity_ += Vector3(0, 0.4f, 0);
}

void Player::ShieldEnemyOnCollsion(const ShieldEnemy* shieldEnemy) {
	if (isAttack()) {
		return;
	}
	(void)shieldEnemy;
	isDead_ = true;
	velocity_ += Vector3(0, 0.4f, 0);
}

float Player::EaseOut(float start, float end, float t) {
	// t は 0〜1 の値
	// Quadratic EaseOut の計算
	return start + (end - start) * (1.0f - (1.0f - t) * (1.0f - t));
}

const bool Player::isAttack() {

	if (behavior_ == Behavior::kAttack) {
		return true;
	}

	return false;
}

// Player.h に宣言
void Player::IsKnockBack() {
	isKnockBack_ = true;
	kKnockBack_ = KnockbackPhase::kStartKnockback; // フェーズをリセット
}
