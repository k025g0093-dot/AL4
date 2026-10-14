#define NOMINMAX
#include "BossEnemy.h"
#include "MapChipField.h"
#include "MyMath.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include "Player.h"
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
	worldTransform_.translation_.y = 5.0f;
	worldTransform_.translation_.x = 10.0f;
	// 見た目の大きさを当たり判定に合わせる（ずれると壁にめり込んで見える）
	worldTransform_.scale_ = {kWidth, kHight, kWidth};

	// 最初は右へ進む
	velocity_ = {kMoveSpeed, 0.0f, 0.0f};
}

void BossEnemy::Update() {

	Move();

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

void BossEnemy::OnCollsion( Player* player) {
	if (player->isAttack()&&!player->status_.isHit) {

		bossState.HP_ -= 10;
		player->status_.isHit = true;
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

	//  今の速度で動いたらどこに当たるか調べて、めり込まない速度に直す
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity_ = velocity_;
	MapCollsion(collisionMapInfo);

	//  直した速度で動く
	CheckedMove(collisionMapInfo);

	//  壁に当たっていたら向きを反転する（次のフレームから逆に進む）
	BounceOnMap(collisionMapInfo, kRestitution);
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
