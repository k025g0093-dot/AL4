#include "GamePlayScene.h"
using namespace KamataEngine;

void GamePlayScene::InitIalize() {

	// 各主newを行う
	fade_ = new Fade;
	player_ = new Player;

	// システムの初期化
	worldTransform_.Initialize();
	camera_.Initialize();

	// インスタンス取得
	input_ = Input::GetInstance();

	// モデル読み込み
	modelPlayer_ = Model::CreateFromOBJ("player", true);

	// キャラクター以外の初期化
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// 各キャラクターの初期化
	player_->Initialize(modelPlayer_, modelPlayer_, &camera_, {0.0f, 0.0f, 0.0f});
}

void GamePlayScene::Updete() {
	fade_->Update();

	switch (phese_) {
	case Phese::kFadeIn:
		if (fade_->IsFinished()) {
			phese_ = Phese::kMain;
		}
		break;

	case Phese::kMain: {

		RanGamePlayScene();

		bool start = Input::GetInstance()->TriggerKey(DIK_SPACE);

		XINPUT_STATE joyState{};
		if (Input::GetInstance()->GetJoystickState(0, joyState)) {
			if (joyState.Gamepad.wButtons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_START)) {
				start = true;
			}
		}

		if (start) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phese_ = Phese::kFadeOut;
		}
		break;
	}

	case Phese::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	default:
		break;
	}
}

void GamePlayScene::Draw() {
	// 描画の開始位置
	Model::PreDraw();
	player_->Draw();
	Model::PostDraw();
}

GamePlayScene::~GamePlayScene() {
	delete fade_;
	delete player_;
}

void GamePlayScene::RanGamePlayScene() {

	player_->Update();

}