#pragma once
#include "KamataEngine.h"


class Player;

class CameraController {

public:
	//// コンストラクタトデストラクタ
	//CameraController();
	//~CameraController();

	KamataEngine::Camera& GetCamera() { return camera_; }//getter

	//矩形
	struct Rect {
		float left =0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	Rect movableArea_ = {0, 100, 0, 100};

// 左右に5、上下に5の「箱」をイメージする場合
	static inline const Rect cameraDeadZone_ = {-5.0f, 5.0f, -5.0f, 5.0f};


	void SetMovableArea(Rect area) { movableArea_ = area; };

	// 初期化関数
	void Initialize();

	// 更新処理関数
	void Update();

	// デバック時に使用するためのもの、基本は使わない
#ifdef _DEBUG
	void Draw();

#endif // DEBUG

	//リセット用の関数
	void Reset();

	//追従対象のカメラの座標（オフセット）
	KamataEngine::Vector3 targetOffset_ = {0, 0, -15.0f};

	KamataEngine::Vector3 endCameraPosition_;

	//外部情報をセットするためのセッター
	void SetTarget(Player* target) { target_ = target; }

	Player* target_ = nullptr;

private:
	// カメラ
	KamataEngine::Camera camera_;

	//座標補間の割合
	static inline const float kInterpolationRate = 0.1f;
	//速度掛け率
	static inline const float kVelocityBias = 20.0f;

	//自作関数
	KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2, float t);
};
