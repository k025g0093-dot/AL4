#include "TitleScene.h"

using namespace KamataEngine;

void TitleScene::InitIalize() {

	fade_ = new Fade;
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	worldTransform_.Initialize();
	camera_.Initialize();


	input_ = Input::GetInstance();

	// スタートの案内の画像
	UITexture_ = TextureManager::Load("SpaceUI.png");
	UISprite_ = Sprite::Create(UITexture_, {0, 0});
	UISprite_->SetSize(Vector2(1280.0f, 720.0f));

	titletexture_ = TextureManager::Load("TITLE.png");
	titleSprite_ = Sprite::Create(titletexture_, {0, 0});
	titleSprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 天球
	modelSkyDome_ = Model::CreateFromOBJ("skydome", true);
	skyDome_ = new SkyDome();
	skyDome_->Initialize(modelSkyDome_);

	// BGM（ループ再生）
	bgmHandle_ = KamataEngine::Audio::GetInstance()->LoadWave("sound/TitleAndSelect.wav");
	bgmVoice_ = KamataEngine::Audio::GetInstance()->PlayWave(bgmHandle_, true, kBgmVolume);
}

void TitleScene::Updete() {

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

void TitleScene::Draw() {
	// 天球を一番奥に描く
	Model::PreDraw();
	skyDome_->Draw(&camera_);
	Model::PostDraw();

	// 画像を描いてから、一番手前にフェードを重ねる
	Sprite::PreDraw();
	titleSprite_->Draw();

	UISprite_->Draw();
	Sprite::PostDraw();

	fade_->Draw();
}

TitleScene::~TitleScene() {
	// シーンが終わるときにBGMを止める（止めないと次のシーンの音と重なる）
	KamataEngine::Audio::GetInstance()->StopWave(bgmVoice_);
	delete fade_;
	delete UISprite_;
	delete titleSprite_;
	delete skyDome_;
	delete modelSkyDome_;
}