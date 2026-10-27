#include "GameClear.h"
using namespace KamataEngine;


void GameClear::InitIalize() {

	fade_ = new Fade;
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	worldTransform_.Initialize();
	camera_.Initialize();

	input_ = Input::GetInstance();

		UITexture_ = TextureManager::Load("SpaceUI.png");
	UISprite_ = Sprite::Create(UITexture_, {0, 0});
	UISprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 結果表示の画像
	resultTexture_ = TextureManager::Load("GAMECLEAR.png");
	resultSprite_ = Sprite::Create(resultTexture_, {0, 0});
	resultSprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 天球
	modelSkyDome_ = Model::CreateFromOBJ("skydome", true);
	skyDome_ = new SkyDome();
	skyDome_->Initialize(modelSkyDome_);

	// BGM（ループ再生）
	bgmHandle_ = KamataEngine::Audio::GetInstance()->LoadWave("sound/GameClear.wav");
	bgmVoice_ = KamataEngine::Audio::GetInstance()->PlayWave(bgmHandle_, true, kBgmVolume);
}

void GameClear::Updete() {

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

void GameClear::Draw() {
	// 天球を一番奥に描く
	Model::PreDraw();
	skyDome_->Draw(&camera_);
	Model::PostDraw();

	// 画像を描いてから、一番手前にフェードを重ねる
	Sprite::PreDraw();
	resultSprite_->Draw();
	UISprite_->Draw();

	Sprite::PostDraw();

	fade_->Draw();
}

GameClear::~GameClear() {
	// シーンが終わるときにBGMを止める（止めないと次のシーンの音と重なる）
	KamataEngine::Audio::GetInstance()->StopWave(bgmVoice_);
	delete fade_;
	delete resultSprite_;
	delete skyDome_;
	delete modelSkyDome_;
	delete UISprite_;
}