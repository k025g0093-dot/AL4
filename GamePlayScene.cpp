#include "GamePlayScene.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using namespace KamataEngine;

void GamePlayScene::InitIalize() {

	// 各主newを行う
	fade_ = new Fade;
	player_ = new Player;
	bossEnemy_ = new BossEnemy;
	mapChipField_ = new MapChipField;
	stageManager_ = new stageManager;
	cameraController_ = new CameraController;
	debugCamera_ = new DebugCamera(1280, 720);
	skyDome_ = new SkyDome();
	deathParticles_ = new DeathParticles();
#pragma region 基礎システムの初期化
	worldTransform_.Initialize();
	camera_.Initialize();
	camera_.farZ = 2000;
#pragma endregion

	// インスタンス取得
	input_ = Input::GetInstance();

	// モデル読み込み
	modelMapBlock_ = Model::CreateFromOBJ("block", true); // マップのモデル

	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelBoss_ = Model::CreateFromOBJ("player", true);
	modelAttack_ = Model::CreateFromOBJ("hit_effect", true); // プレイヤーの攻撃モデル
	modelSkyDome_ = Model::CreateFromOBJ("skydome", true);

	// キャラクター以外の初期化
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// 各キャラクターの初期化

	skyDome_->Initialize(modelSkyDome_);

#pragma region カメラコントローラーの設定
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);

	// ここで距離の微調整が可能
	cameraController_->targetOffset_ = {0.0f, 0.0f, -20.0f};
	cameraController_->SetMovableArea({11.0f, 100.0f, 6.0f, 100.0f}); // カメラの移動できる最大値、最少値
	cameraController_->Reset();

	// ライン描画用カメラをコントローラー側に同期
	PrimitiveDrawer::GetInstance()->SetCamera(&cameraController_->GetCamera());
#pragma endregion

#pragma region マップ・オブジェクトの初期化
	// マップチップ読み込み
	stageManager_->LoadStageDataCsv();
	const StageData& stageData = stageManager_->GetCurrentStageIndex();
	std::string stageFileName = "Resources/fields/" + stageData.name + ".csv";
	mapChipField_->LoadMapChipCsv(stageFileName);

	GenerateFieldObjects();
#pragma endregion

	bossEnemy_->Initialize(&cameraController_->GetCamera(), modelBoss_);
	bossEnemy_->SetMapChipField(mapChipField_); // マップのデータをボスに渡す
}

void GamePlayScene::Updete() {
	fade_->Update();

	switch (phese_) {
	case Phese::kFadeIn:

		StageUpdate();

		if (fade_->IsFinished()) {
			phese_ = Phese::kMain;
		}

		break;

	case Phese::kMain: {

		RanGamePlayScene();

		bool start = Input::GetInstance()->TriggerKey(DIK_P);

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
	case Phese::kDeht: // 死亡時
	{

		// ゲームオーバ中の処理
		DeathUpdate();
		// 右端の処理：パーティクルもフェードも終わったらシーン終了
		if (deathParticles_->IsFinished() && fade_->IsFinished()) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phese_ = Phese::kFadeOut;
		}
	} break;

	case Phese::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	default:
		break;
	}

	DrawDebugGui();
}

void GamePlayScene::DrawDebugGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Status");

	// プレイヤー
	ImGui::Text("Player HP : %d", player_->status_.HP);
	ImGui::ProgressBar(static_cast<float>(player_->status_.HP) / 5.0f, ImVec2(200.0f, 0.0f));
	const Vector3& playerPos = player_->GetWorldTransform().translation_;
	const Vector3& playerVel = player_->GetVelocity();
	ImGui::Text("Player Pos : (%.2f, %.2f)", playerPos.x, playerPos.y);
	ImGui::Text("Player Vel : (%.2f, %.2f)", playerVel.x, playerVel.y);

	ImGui::Separator();

	// ボス
	ImGui::Text("Boss HP : %d", bossEnemy_->GetHP());
	ImGui::ProgressBar(static_cast<float>(bossEnemy_->GetHP()) / 100.0f, ImVec2(200.0f, 0.0f));
	const Vector3& bossPos = bossEnemy_->GetWorldTransform().translation_;
	ImGui::Text("Boss Pos : (%.2f, %.2f)", bossPos.x, bossPos.y);

	ImGui::End();
#endif
}

void GamePlayScene::Draw() {
	// コントローラー側のカメラを取得
	Camera& activeCamera = cameraController_->GetCamera();

	// 描画の開始位置
	Model::PreDraw();
	skyDome_->Draw(&activeCamera);

#pragma region マップ描画
	for (std::vector<WorldTransform*>& worldTransformBlockRow : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockRow) {
			if (worldTransformBlock) {
				modelMapBlock_->Draw(*worldTransformBlock, activeCamera);
			}
		}
	}
#pragma endregion
	player_->Draw();
	bossEnemy_->Draw();
	if (!deathParticles_->isFinished_) {
		deathParticles_->Draw();
	}
	Model::PostDraw();

	fade_->Draw();
}

GamePlayScene::~GamePlayScene() {
	delete fade_;
	delete player_;
	delete modelAttack_;
	delete modelPlayer_;
	delete debugCamera_;
	delete skyDome_;
	delete modelBoss_;
	delete deathParticles_;

	for (std::vector<WorldTransform*>& worldTransformBlockRow : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockRow) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}

void GamePlayScene::StageUpdate() {

	cameraController_->Update();

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockRow : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockRow) {

			if (!worldTransformBlock) {
				continue;
			}

			// 4. アフィン変換行列の合成 (Scale * Rotate * Translate)
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

			// 5. 行列を定数バッファに転送
			worldTransformBlock->TransferMatrix();
		}
	}
}

void GamePlayScene::RanGamePlayScene() {

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		isDebugCamera_ = true;
	}
#endif // DEBUG
	if (isDebugCamera_) {
		// デバックカメラの更新処理
		debugCamera_->Update();

		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}
	skyDome_->Update();
	player_->Update();
	bossEnemy_->Update();

	CheckAllCollisions();

	if (player_->IsDead()) {

		phese_ = Phese::kDeht;
	
		deathParticles_->Initialize(modelPlayer_, &cameraController_->GetCamera(), player_->GetWorldPodition());

	}

	// カメラの更新処理
	cameraController_->Update();
}

void GamePlayScene::GenerateFieldObjects() {
	// 要素数
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t mumBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	// 要素の変更
	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		worldTransformBlocks_[i].resize(mumBlockHorizontal); // 各列に「行」を確保する
	}

	// キューブ生成
	// GameScene.cpp 修正後
	for (uint32_t j = 0; j < numBlockVirtical; j++) {
		for (uint32_t i = 0; i < mumBlockHorizontal; i++) {

			MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(i, j);

			switch (mapChipType) {
			case MapChipType::kBlock: {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();

				worldTransformBlocks_[j][i] = worldTransform;

				worldTransformBlocks_[j][i]->translation_ = mapChipField_->GetMapChipPositionByIndex(i, j);
			} break;
			case MapChipType::kPlayer: {
				Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(i, j);
				player_->Initialize(modelPlayer_, modelAttack_, &cameraController_->GetCamera(), playerPosition);
				player_->SetMapChipFiled(mapChipField_); // マップのデータをプレイヤーに渡す

				deathParticles_->Initialize(modelPlayer_, &cameraController_->GetCamera(), playerPosition);

			} break;
			}
		}
	}
}

void GamePlayScene::CheckAllCollisions() { BossToPlayerAABB(); }

void GamePlayScene::BossToPlayerAABB() {
	AABB aabb1, aabb2;
	aabb1 = player_->GetAABB();
	aabb2 = bossEnemy_->GetAABB();
	if (IsCollision(aabb1, aabb2)) {
		player_->EnemyOnCollsion(bossEnemy_);
		bossEnemy_->OnCollsion(player_);
	}
}

// デス演出更新処理
void GamePlayScene::DeathUpdate() {

	bossEnemy_->Update();


	// スカイドームの更新処理
	skyDome_->Update();

	// パーティクルの処理
	if (!deathParticles_->isFinished_) {
		deathParticles_->Update();
	}
}
