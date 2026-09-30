#include "Fade.h"
#include <algorithm>

using namespace KamataEngine;

void Fade::Initialize() {

	textureHandle_ = TextureManager::Load("white1x1.png");
	sprite_ = Sprite::Create(textureHandle_, {0, 0});
	sprite_->SetSize(Vector2(1280, 720));
	sprite_->SetColor(Vector4(0, 0, 0, 1));
}

void Fade::Start(Status status, float duration) {

	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Update() {

	switch (status_) {
	case Fade::Status::None:
		// 何もしない

		break;
	case Fade::Status::FadeIn:
		// フェードイン
		//  フェードアウト
		counter_ += 1.0f / 60.0f;

		if (counter_ >= duration_) {
			counter_ = duration_;
		}

		sprite_->SetColor(Vector4(0, 0, 0, std::clamp((duration_ - counter_) / duration_, 0.0f, 1.0f)));

		break;
	case Fade::Status::FadeOut:
		// フェードアウト
		counter_ += 1.0f / 60.0f;

		if (counter_ >= duration_) {
			counter_ = duration_;
		}

		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));

		break;
	default:
		break;
	}
}

void Fade::Draw() {
	Sprite::PreDraw();
	if (status_ == Status::None) {
		return;
	}
	sprite_->Draw();
	Sprite::PostDraw();
}

void Fade::Stop() { status_ = Status::None; }

bool Fade::IsFinished() const {
	switch (status_) {
	case Fade::Status::None:
		break;
	case Fade::Status::FadeIn:
		if (counter_ >= duration_) {
			return true;
		} else {
			return false;
		}
		break;
	case Fade::Status::FadeOut:

		if (counter_ >= duration_) {
			return true;
		} else {

			return false;
		}

		break;
	default:
		break;
	}
	return true;
}