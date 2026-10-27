#define NOMINMAX
#include "BossEnemy.h"
#include "MapChipField.h"
#include "MyMath.h"

#include "Player.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

BossEnemy::BossEnemy() = default;

BossEnemy::~BossEnemy() = default;

void BossEnemy::Initialize(Camera* camera, Model* model) {

	assert(camera);
	assert(model);

	camera_ = camera;
	model_ = model;

	bossState.HP_ = 100;

	worldTransform_.Initialize();
	// 大きさ2でもブロックに重ならない、開けた場所に置く
	worldTransform_.translation_.x = 28.0f;
	worldTransform_.translation_.y = 11.0f;
	// 見た目の大きさを当たり判定に合わせる（ずれると壁にめり込んで見える）
	worldTransform_.scale_ = {kWidth, kHight, kWidth};

	BehaviorAttackInitialize();

	// 最初は右上へ進む（斜めでも速さが kMoveSpeed になるように、向きの長さを1にしてから掛ける）
	float rdian = RandomEngine::GetRandomFloat(0, std::numbers::pi_v<float> * 2.0f);
	const float kDiagonalX = std::cos(rdian);
	const float kDiagonalY = std::sin(rdian);
	velocity_ = {kMoveSpeed * kDiagonalX, kMoveSpeed * kDiagonalY, 0.0f};
}

void BossEnemy::Update() {

	if (bossState.HP_ <= 0) {
		bossState.isDeth = true;
	}

	BehaviorUpdate();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void BossEnemy::Draw() { model_->Draw(worldTransform_, *camera_); }

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

void BossEnemy::OnCollsion(Player* player) {
	if (player->isAttack() && !player->status_.isHit) {

		bossState.HP_ -= 10;
		player->status_.isHit = true;

		// 最初は右上へ進む（斜めでも速さが kMoveSpeed になるように、向きの長さを1にしてから掛ける）
		float rdian = RandomEngine::GetRandomFloat(0, std::numbers::pi_v<float> * 2.0f);
		const float kDiagonalX = std::cos(rdian);
		const float kDiagonalY = std::sin(rdian);
		velocity_ = {(kMoveSpeed * kDiagonalX) * 8.0f, (kMoveSpeed * kDiagonalY) * 8.0f, 0.0f};

		return;
	}
}

// ============================================================
// 移動
// ============================================================

void BossEnemy::Move() {
	// マップを受け取っていなければ動かない（SetMapChipFieldの呼び忘れ対策）
	if (!mapChipField_) {
		return;
	}

	Vector3& position = worldTransform_.translation_;

	// 横と縦を別々に動かす。
	// 同時に動かすと、斜めにブロックの角へ入ったときに「横と縦のどちらで当たったか」が分からなくなるため。

	//  横だけ動かして、重なったら戻して横の向きを反転
	position.x += velocity_.x;
	if (HitsBlock(position)) {
		position.x -= velocity_.x;
		velocity_.x = -velocity_.x * kRestitution;
	}

	//  縦だけ動かして、重なったら戻して縦の向きを反転
	position.y += velocity_.y;
	if (HitsBlock(position)) {
		position.y -= velocity_.y;
		velocity_.y = -velocity_.y * kRestitution;
	}
}

void BossEnemy::ChooseRandomDirection() {
	// ランダムで進むようにする

	float rdian = RandomEngine::GetRandomFloat(0, std::numbers::pi_v<float> * 2.0f);
	const float kDiagonalX = std::cos(rdian);
	const float kDiagonalY = std::sin(rdian);
	velocity_ = {kMoveSpeed * kDiagonalX, kMoveSpeed * kDiagonalY, 0.0f};
}

bool BossEnemy::HitsBlock(const Vector3& center) const {
	// ちょうど辺が接しているだけのときは重なりにしない（ほんの少しだけ内側で調べる）
	const float kEpsilon = 0.001f;
	const float left = center.x - kWidth / 2.0f + kEpsilon;
	const float right = center.x + kWidth / 2.0f - kEpsilon;
	const float bottom = center.y - kHight / 2.0f + kEpsilon;
	const float top = center.y + kHight / 2.0f - kEpsilon;

	// マップの外に出ていたら壁として扱う（マス番号がマイナスになって壊れるのを防ぐ）
	const float mapLeft = -MapChipField::kBlockWidth / 2.0f;
	const float mapBottom = -MapChipField::kBlockHeight / 2.0f;
	const float mapRight = MapChipField::kBlockWidth * static_cast<float>(MapChipField::kNumBlockHorizontal) + mapLeft;
	const float mapTop = MapChipField::kBlockHeight * static_cast<float>(MapChipField::kNumBlockVirtical) + mapBottom;
	if (left < mapLeft || bottom < mapBottom || right >= mapRight || top >= mapTop) {
		return true;
	}

	// ボスの四角が覆うマスを左上から右下まで全部調べる（角だけだと間のブロックを見逃すため）
	MapChipField::IndexSet leftTop = mapChipField_->GetMapChipIndexSetByPosition({left, top, 0.0f});
	MapChipField::IndexSet rightBottom = mapChipField_->GetMapChipIndexSetByPosition({right, bottom, 0.0f});

	for (uint32_t y = leftTop.yIndex; y <= rightBottom.yIndex; ++y) {
		for (uint32_t x = leftTop.xIndex; x <= rightBottom.xIndex; ++x) {
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kBlock) {
				return true;
			}
		}
	}
	return false;
}


// ============================================================
// ボスの行動変化
// ============================================================

void BossEnemy::BehaviorUpdate() {

	if (bossState.isDeth) {return;}

	switch (bossBehaviro) {
	case BossBehavior::kRoot:

		//ランダムに動いて歩き回る処理
		if (directionChangeTimer <= 0) {
			ChooseRandomDirection();
			directionChangeTimer = RandomEngine::GetRandomInt(kDirectionChangeMinFrames, kDirectionChangeMaxFrames);
		}
		directionChangeTimer--;

		if (directionChangePaseTimer <= 0) {
			bossBehaviro = BossBehavior::kAttack;
			directionChangePaseTimer = RandomEngine::GetRandomInt(kDirectionChangeMinPhase, kDirectionChangeMaxPhase);
		}
		directionChangePaseTimer--;

		Move();

		break;
	case BossBehavior::kAttack:

		BehaviorAttackUpdate();
		AttackMapCollision();

		if (attack_.phase == BossAttackPhase::kAfterglow) {
			bossBehaviro = BossBehavior::kUnknown;
			stunTimer = kStunFrames;
		}

		break;
	case BossBehavior::kUnknown:

		if (stunTimer <= 0) {
			BehaviorAttackInitialize();
			bossBehaviro = BossBehavior::kRoot;
			stunTimer = 0;
		}
		stunTimer--;

		break;
	default:
		break;
	}

}

// ============================================================
// 攻撃処理
// ============================================================

void BossEnemy::BehaviorAttackInitialize() { attack_.phase = BossAttackPhase::kSave; }

void BossEnemy::BehaviorAttackUpdate() {

	switch (attack_.phase) {
	case BossAttackPhase::kSave: {

		float rdian = RandomEngine::GetRandomFloat(0, std::numbers::pi_v<float> * 2.0f);
		const float kDiagonalX = std::cos(rdian);
		const float kDiagonalY = std::sin(rdian);

		float t = static_cast<float>(attack_.counter) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
		attack_.counter++;
		if (attack_.counter >= 10) {
			attack_.phase = BossAttackPhase::kCharge;
			attack_.counter = 0;
			attack_.direction.x = kDiagonalX;
			attack_.direction.y = kDiagonalY;
			// 突進の向きを確定させる（斜めでも速さが変わらないように長さを1にする）
			float length = std::sqrt(attack_.direction.x * attack_.direction.x + attack_.direction.y * attack_.direction.y);
			if (length > 0.0f) {
				attack_.direction.x /= length;
				attack_.direction.y /= length;
			}
		}

		break;
	}
	case BossAttackPhase::kCharge: {
		attack_.counter++;
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
	case BossAttackPhase::kAfterglow: { // 余韻
		float t = static_cast<float>(attack_.counter) / 10.0f;
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
		break;
	}
	default:
		break;
	}
}

void BossEnemy::AttackMapCollision() {
	// マップを受け取っていなければ動かない（SetMapChipFieldの呼び忘れ対策）
	if (!mapChipField_) {
		return;
	}

	Vector3& position = worldTransform_.translation_;

	// 横と縦を別々に動かす。
	// 同時に動かすと、斜めにブロックの角へ入ったときに「横と縦のどちらで当たったか」が分からなくなるため。

	//  横だけ動かして、重なったら戻して横の向きを反転
	position.x += velocity_.x;

	if (HitsBlock(position)) {
		position.x -= velocity_.x;
		velocity_.x = -velocity_.x * kRestitution;
		attack_.phase = BossAttackPhase::kAfterglow;
	}
	position.y += velocity_.y ;

	//  縦だけ動かして、重なったら戻して縦の向きを反転
	if (HitsBlock(position)) {
		position.y -= velocity_.y;
		velocity_.y = -velocity_.y * kRestitution;
		attack_.phase = BossAttackPhase::kAfterglow;
	}
}

// ============================================================
// マップとの当たり判定
// ============================================================

void BossEnemy::MapCollsion(CollisionMapInfo& info) {
	CheckCollisionTop(info);
	CheckCollisionBottom(info);
	CheckCollisionRight(info);
	CheckCollisionLeft(info);
}

// ---------- 右 ----------
void BossEnemy::CheckCollisionRight(CollisionMapInfo& info) {
	if (info.velocity_.x <= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<BossCorner>(i));
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

// ---------- 左 ----------
void BossEnemy::CheckCollisionLeft(CollisionMapInfo& info) {
	if (info.velocity_.x >= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<BossCorner>(i));
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

// ---------- 上 ----------
void BossEnemy::CheckCollisionTop(CollisionMapInfo& info) {
	if (info.velocity_.y <= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<BossCorner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	BossCorner hitCorner = BossCorner::kBossCornerLeftTop; // 当たった角

	// 左上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = BossCorner::kBossCornerLeftTop;
	}

	// 右上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = BossCorner::kBossCornerRightTop;
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
void BossEnemy::CheckCollisionBottom(CollisionMapInfo& info) {
	if (info.velocity_.y > 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPositio(worldTransform_.translation_ + info.velocity_, static_cast<BossCorner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	MapChipField::IndexSet indexSet;
	bool hit = false;
	BossCorner hitCorner = BossCorner::kBossCornerLeftBottom; // 当たった角

	// 左下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = BossCorner::kBossCornerLeftBottom;
	}

	// 右下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
		hitCorner = BossCorner::kBossCornerRightBottom;
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
void BossEnemy::CheckedMove(const CollisionMapInfo& info) { worldTransform_.translation_ += info.velocity_; }

// 左右の壁に当たったら横を、床か天井に当たったら縦を反転する（角なら両方）
void BossEnemy::BounceOnMap(const CollisionMapInfo& info, float restitution) {
	if (info.isWall) {
		velocity_.x = -velocity_.x * restitution;
	}
	if (info.isCeiling || info.isFloor) {
		velocity_.y = -velocity_.y * restitution;
	}
}

// 壁に当たったら横の速度を減らす
void BossEnemy::IsHitWall(const CollisionMapInfo& info) {
	if (info.isWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// 天井に当たったら縦の速度を0にする
void BossEnemy::IsHitTop(const CollisionMapInfo& info) {
	if (info.isCeiling) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0;
	}
}

// 床に当たったら横を減速して縦を0にする
void BossEnemy::IsGrounded(const CollisionMapInfo& info) {
	if (info.isFloor) {
		DebugText::GetInstance()->ConsolePrintf("isFloor landing\n");
		velocity_.x *= (1.0f - kAttenuationLanding);
		velocity_.y = 0.0f;
	}
}

// 中心から角の位置を求める
Vector3 BossEnemy::CornerPositio(const Vector3& center, BossCorner corner) {
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


float BossEnemy::EaseOut(float start, float end, float t) { return start + (end - start) * (1.0f - (1.0f - t) * (1.0f - t)); }
