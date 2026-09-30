#include "AllSceneInclude.h"
#include "KamataEngine.h"
#include <Windows.h>

using namespace KamataEngine;

GamePlayScene* gamePlay_ = nullptr;
TitleScene* title_ = nullptr;
GameClear* clear_ = nullptr;
GameOver* gameOver_ = nullptr;

enum class Scene { kUnknown = 0, kTitle, kGame, kGameClear, kGameOver };

Scene scene = Scene::kTitle;

void ChangeScene();
void UpdateScene();
void DrawScene();

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	KamataEngine::Initialize(L"LE2B_29_ヤマト_ユウヤ_味方はぜんぶ、元は敵");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	title_ = new TitleScene;
	title_->InitIalize();

	while (true) {

		if (KamataEngine::Update()) {
			break;
		}

		// 更新処理

		ChangeScene();
		UpdateScene();

		// 描画処理
		dxCommon->PreDraw();

		DrawScene();

		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	return 0;
}

void ChangeScene() {

	switch (scene) {
	case Scene::kTitle:

		if (title_->IsFinished()) {
			scene = Scene::kGame;
			delete title_;
			title_ = nullptr;

			gamePlay_ = new GamePlayScene;
			gamePlay_->InitIalize();
		}

		break;
	case Scene::kGame:

		if (gamePlay_->IsFinished()) {

			scene = Scene::kGameClear;
			delete gamePlay_;
			gamePlay_ = nullptr;

			clear_ = new GameClear;
			clear_->InitIalize();
		}

		break;
	case Scene::kGameClear:

		if (clear_->IsFinished()) {

			scene = Scene::kTitle;
			delete clear_;
			clear_ = nullptr;

			title_ = new TitleScene;
			title_->InitIalize();
		}

		break;
	case Scene::kGameOver:

		if (gameOver_->IsFinished()) {

			scene = Scene::kTitle;
			delete gameOver_;
			gameOver_ = nullptr;

			title_ = new TitleScene;
			title_->InitIalize();

		}

		break;
	default:
		break;
	}
}

#pragma region ゲームの更新処理
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle: // タイトル
		// LoadDebugSettings();

		title_->Updete();
		break;
	case Scene::kGame: // ゲーム中
		// LoadDebugSettings();

		gamePlay_->Updete();
		break;
	case Scene::kGameClear: // ゲームクリア
		clear_->Updete();
		break;

	case Scene::kGameOver: // ゲームオーバー
		gameOver_->Updete();
		break;

	default:
		break;
	}
}
#pragma endregion

#pragma region ゲームの描画
void DrawScene() {
	switch (scene) {
	case Scene::kTitle: // タイトル
		// LoadDebugSettings();

		title_->Draw();
		break;
	case Scene::kGame: // ゲーム中
		// LoadDebugSettings();

		gamePlay_->Draw();
		break;
	case Scene::kGameClear: // ゲームクリア
		clear_->Draw();

		break;

	case Scene::kGameOver: // ゲームオーバー
		gameOver_->Draw();

		break;

	default:
		break;
	}
}
#pragma endregion