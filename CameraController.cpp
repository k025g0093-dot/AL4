#define NOMINMAX
#include "CameraController.h"
#include "KamataEngine.h"
#include "MyMath.h"

#include "Player.h"
#include <algorithm>
#include <numbers>

//カマタエンジンの名前の省略のために宣言
using namespace KamataEngine;

//CameraController::CameraController() { 
//	Initialize(); 
//}

//カメラの初期化関数
void CameraController::Initialize() { 
	//カメラの初期化
	camera_.Initialize();

	camera_.fovAngleY = 0.6f;

	camera_.UpdateMatrix();

}

void CameraController::Update() {

	Vector3 targetVelocity = target_->GetVelocity();

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& tragetWorldTransform = target_->GetWorldTransform();


	// 1. 加速度によるオフセットを計算
	Vector3 velocityOffset = {
		targetVelocity.x * kVelocityBias,
		targetVelocity.y * kVelocityBias, 
		targetVelocity.z * kVelocityBias
	};


	// 2. それを足し合わせる
	endCameraPosition_ = tragetWorldTransform.translation_ + targetOffset_ + velocityOffset;


	//座標補間によりゆったりと追従
	camera_.translation_ = Lerp(camera_.translation_, endCameraPosition_, kInterpolationRate);

	//追従対象と画面外に出ないように補正
	//X軸
	camera_.translation_.x = std::max(camera_.translation_.x, tragetWorldTransform.translation_.x + cameraDeadZone_.left);
	camera_.translation_.x = std::min(camera_.translation_.x, tragetWorldTransform.translation_.x + cameraDeadZone_.right);
	//Y軸
	camera_.translation_.y = std::max(camera_.translation_.y, tragetWorldTransform.translation_.y + cameraDeadZone_.bottom);
	camera_.translation_.y = std::min(camera_.translation_.y, tragetWorldTransform.translation_.y + cameraDeadZone_.top);


	//移動範囲制限処理
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

	//行列の更新
	camera_.UpdateMatrix();

}

void CameraController::Reset() {

	//追従対象のワールドトランスフォームを参照
	const WorldTransform& tragetWorldTransform = target_->GetWorldTransform();

	//追従対象とオフセットからカメラの座標を計算
	camera_.translation_ = tragetWorldTransform.translation_ + targetOffset_;

}


// 自作 Lerp 関数の実装
Vector3 CameraController::Lerp(const Vector3& v1, const Vector3& v2, float t) {
	Vector3 res;
	res.x = v1.x + (v2.x - v1.x) * t;
	res.y = v1.y + (v2.y - v1.y) * t;
	res.z = v1.z + (v2.z - v1.z) * t;
	return res;
}

//CameraController::~CameraController() { 
//}