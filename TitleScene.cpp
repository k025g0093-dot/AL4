#include "TitleScene.h"

using namespace KamataEngine;

void TitleScene::InitIalize() {

	fade_ = new Fade;
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	worldTransform_.Initialize();
	camera_.Initialize();

	input_ = Input::GetInstance();
}

void TitleScene::Updete() {

	fade_->Update();

	switch (phese_) {
	case Phese::kFadeIn:
		if (fade_->IsFinished()) {
			phese_ = Phese::kMain;
		}
		break;

	case Phese::kMain: {
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

void TitleScene::Draw() { fade_->Draw(); }

TitleScene::~TitleScene() { delete fade_; }