#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include <cmath>

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

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::Update() {

	// ヒットストップ中は何も進めない（行列は前のフレームのものがそのまま使われる）
	if (hitStopTimer_ > 0) {
		hitStopTimer_--;
		return;
	}

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

		if (kAttackPhase_ == AttackPhase::kCharge &&
			(collisionMapInfo.isWall || collisionMapInfo.isCeiling || collisionMapInfo.isFloor)) {
			kAttackPhase_ = AttackPhase::kAfterglow;
			attackCounter_ = 0; // 余韻を0から数え直す

			// 当たった面で突進の向きを反射して、跳ね返りの速度にする
			Vector3 reflect = kAttakDirection;
			if (collisionMapInfo.isWall) {
				reflect.x = -reflect.x;
			}
			if (collisionMapInfo.isCeiling || collisionMapInfo.isFloor) {
				reflect.y = -reflect.y;
			}
			velocity_ = reflect * kBounceSpeed;
			//hitStopTimer_ = kHitStopFrames; // 一瞬止めてから跳ね返る
		}
		else {
			// 余韻中に別の壁に当たったときも跳ね返る
			BounceOnMap(collisionMapInfo, kBounceRestitution);
		}

		break;

		// default:
		//	break;
	}

	// 余韻が終わったら通常状態へ（跳ね返りの速度は残したまま）
	if (kAttackPhase_ == AttackPhase::kAfterglow && attackCounter_ >= 10) {
		kAttakDirection = { 0, 0, 0 };
		worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
		kAttackPhase_ = AttackPhase::kSave; // 次の攻撃に備えてリセット
		behaviorRequest_ = Behavior::kRoot;
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

	BounceOnMap(collisionMapInfo, kBounceRestitution);
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
		attackCounter_++;
		if (attackCounter_ >= 10) {
			kAttackPhase_ = AttackPhase::kCharge;
			attackCounter_ = 0;

			// 突進の向きを確定させる（斜めでも速さが変わらないように長さを1にする）
			float length = std::sqrt(kAttakDirection.x * kAttakDirection.x + kAttakDirection.y * kAttakDirection.y);
			if (length > 0.0f) {
				kAttakDirection.x /= length;
				kAttakDirection.y /= length;
			}
			else {
				// 何も押していなければ、向いている方へ
				kAttakDirection = { lrDirection_ == LRDirection::kRight ? 1.0f : -1.0f, 0.0f, 0.0f };
			}
		}
		break;
	}
	case AttackPhase::kCharge: // 突進動作
	{
		// 出だしが一番速く、だんだん落ちて最低速度で落ち着く
		float t = std::min(static_cast<float>(attackCounter_) / kDashEaseFrames, 1.0f);
		float ease = 1.0f - (1.0f - t) * (1.0f - t);
		float speed = kDashStartSpeed + (kDashMinSpeed - kDashStartSpeed) * ease;

		velocity_.x = kAttakDirection.x * speed;
		velocity_.y = kAttakDirection.y * speed;

		// 進んでいる向きに伸ばす（横の成分はz、縦の成分はyを伸ばす）
		float stretch = 1.0f + 0.5f * (speed / kDashStartSpeed);
		float squash = 1.0f / stretch;
		float ax = std::fabs(kAttakDirection.x);
		float ay = std::fabs(kAttakDirection.y);
		worldTransform_.scale_.z = squash + (stretch - squash) * ax;
		worldTransform_.scale_.y = squash + (stretch - squash) * ay;
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
	Input* input = Input::GetInstance();

	// 突進の向き：押しているキーから毎フレーム作り直す（離した軸が残らないように）
	kAttakDirection = { 0.0f, 0.0f, 0.0f };
	if (input->PushKey(DIK_D)) {
		kAttakDirection.x += 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		kAttakDirection.x -= 1.0f;
	}
	if (input->PushKey(DIK_W)) {
		kAttakDirection.y += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		kAttakDirection.y -= 1.0f;
	}

	// 見た目の向き（左右を押したときだけ旋回）
	if (kAttakDirection.x != 0.0f) {
		LRDirection next = kAttakDirection.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		if (lrDirection_ != next) {
			lrDirection_ = next;
			turnFistRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 1.0f;
		}
	}

	// 無重力：狙う入力では速度を変えず、少しずつ減速しながら漂う
	velocity_.x *= kDriftDamping;
	velocity_.y *= kDriftDamping;
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

void Player::BounceOnMap(const CollisionMapInfo& info, float restitution) {
	// 左右の壁に当たったら横を、床か天井に当たったら縦を反転する（角なら両方）
	if (info.isWall) {
		velocity_.x = -velocity_.x * restitution;
	}
	if (info.isCeiling || info.isFloor) {
		velocity_.y = -velocity_.y * restitution;
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

	if (info.isFloor) { // 修正: isRoof → isFloor
		DebugText::GetInstance()->ConsolePrintf("isFloor landing\n");
		velocity_.x *= (1.0f - kAttenuationLanding);
		velocity_.y = 0.0f;
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
