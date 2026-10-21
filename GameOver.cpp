#include "GameOver.h"

using namespace KamataEngine;

void GameOver::InitIalize() {

	fade_ = new Fade;
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	worldTransform_.Initialize();
	camera_.Initialize();

	input_ = Input::GetInstance();

	UITexture_ = TextureManager::Load("SpaceUI.png");
	UISprite_ = Sprite::Create(UITexture_, {0, 0});
	UISprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 結果表示は3Dモデルをやめて画像にする
	resultTexture_ = TextureManager::Load("GAMEOVER.png");
	resultSprite_ = Sprite::Create(resultTexture_, {0, 0});
	resultSprite_->SetPosition(Vector2(0.0f, 0.0f));
	resultSprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 天球
	modelSkyDome_ = Model::CreateFromOBJ("skydome", true);
	skyDome_ = new SkyDome();
	skyDome_->Initialize(modelSkyDome_);
}

void GameOver::Updete() {
	fade_->Update();
	skyDome_->Update();

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

void GameOver::Draw() {

	// 天球を一番奥に描く
	Model::PreDraw();
	skyDome_->Draw(&camera_);
	Model::PostDraw();

	// 2Dスプライト描画
	Sprite::PreDraw();
	resultSprite_->Draw();
	UISprite_->Draw();

	Sprite::PostDraw();

	// フェードは一番手前に重ねる（先に描くと画像に隠れてフェードが見えない）
	fade_->Draw();


}

GameOver::~GameOver() {
	delete fade_;
	delete resultSprite_;
	delete skyDome_;
	delete modelSkyDome_;
	delete UISprite_;
}