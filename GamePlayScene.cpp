#include "GamePlayScene.h"

#include <cmath>
#include <numbers>

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
	modelBoss_ = Model::CreateFromOBJ("BossEnemy", true);
	modelAttack_ = Model::CreateFromOBJ("hit_effect", true); // プレイヤーの攻撃モデル
	modelSkyDome_ = Model::CreateFromOBJ("skysphere", true); // 天球

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

	InitializeHud();

	// BGM（ループ再生）
	bgmHandle_ = KamataEngine::Audio::GetInstance()->LoadWave("sound/GamePlaye.wav");
	bgmVoice_ = KamataEngine::Audio::GetInstance()->PlayWave(bgmHandle_, true, kBgmVolume);
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
		break;
	}
	case Phese::kDeht: // 死亡時
	{

		// ゲームオーバ中の処理
		DeathUpdate();
		// パーティクルもフェードも終わったらシーン終了
		if (deathParticles_->IsFinished() && fade_->IsFinished()) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			isGameClear = false;
			phese_ = Phese::kFadeOut;
		}
	} break;

	case Phese::kClear: {
		// ゲームクリア中の処理
		DeathUpdate();
		// パーティクルもフェードも終わったらシーン終了
		if (deathParticles_->IsFinished() && fade_->IsFinished()) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			isGameClear = true;
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

	UpdateHud();
	UpdateAimMarker();
	DrawDebugGui();
}

// ============================================================
// 体力のUI
// ============================================================

void GamePlayScene::InitializeHud() {
	hudTexture_ = TextureManager::Load("white1x1.png");

	// ボスの体力バー：暗い背景（枠）の上に、赤い中身を重ねる
	bossHpBack_ = Sprite::Create(hudTexture_, {kBossBarX - kBossBarFrame, kBossBarY - kBossBarFrame});
	bossHpBack_->SetSize(Vector2(kBossBarWidth + kBossBarFrame * 2.0f, kBossBarHeight + kBossBarFrame * 2.0f));
	bossHpBack_->SetColor(Vector4(0.08f, 0.08f, 0.12f, 0.85f));

	bossHpFill_ = Sprite::Create(hudTexture_, {kBossBarX, kBossBarY});
	bossHpFill_->SetSize(Vector2(kBossBarWidth, kBossBarHeight));
	bossHpFill_->SetColor(Vector4(0.9f, 0.2f, 0.3f, 1.0f));

	// プレイヤーの体力：最大体力の数だけ四角を並べる
	for (int i = 0; i < kPlayerMaxHP; ++i) {
		float x = kHpIconX + (kHpIconSize + kHpIconSpace) * static_cast<float>(i);
		playerHpIcons_[i] = Sprite::Create(hudTexture_, {x, kHpIconY});
		playerHpIcons_[i]->SetSize(Vector2(kHpIconSize, kHpIconSize));
	}

	// 突進の向きの目印：体力と同じ白い画像の四角を8つ。真ん中を基準に置く
	// 操作説明：画面の右下
	controlsTexture_ = TextureManager::Load("controlsUI.png");
	controlsSprite_ = Sprite::Create(controlsTexture_, {1280.0f - kControlsWidth - kControlsMargin, 720.0f - kControlsHeight - kControlsMargin});

	for (size_t i = 0; i < aimDots_.size(); ++i) {
		aimDots_[i] = Sprite::Create(hudTexture_, {0.0f, 0.0f});
		aimDots_[i]->SetSize(Vector2(kAimDotSize, kAimDotSize));
		aimDots_[i]->SetAnchorPoint(Vector2(0.5f, 0.5f));
	}
}

void GamePlayScene::UpdateAimMarker() {
	// 突進中と死んだあとは出さない（狙っているときだけ見せる）
	isAimVisible_ = !player_->isAttack() && !player_->IsDead();
	if (!isAimVisible_) {
		return;
	}

	// ワールドの位置を画面の位置（ピクセル）に変える準備
	const Camera& camera = cameraController_->GetCamera();
	const Matrix4x4 viewProjection = Multiply(camera.matView, camera.matProjection);

	const Vector3 center = player_->GetWorldTransform().translation_;
	const Vector3 aim = player_->GetAimDirection();

	// 狙っている向きが8方向のどれに一番近いか（角度を45度ずつに丸める）
	const float kStep = std::numbers::pi_v<float> * 2.0f / static_cast<float>(kAimDotCount);
	float aimAngle = std::atan2(aim.y, aim.x);
	int litIndex = static_cast<int>(std::lround(aimAngle / kStep));
	litIndex = (litIndex % kAimDotCount + kAimDotCount) % kAimDotCount; // マイナスを0〜7に直す

	for (size_t i = 0; i < aimDots_.size(); ++i) {
		// この四角の方向（0番が右、反時計回り）
		float angle = kStep * static_cast<float>(i);
		Vector3 world = center + Vector3(std::cos(angle), std::sin(angle), 0.0f) * kAimDotRadius;

		Vector3 ndc = Transform(world, viewProjection); // -1〜1 の範囲になる
		float screenX = (ndc.x + 1.0f) * 0.5f * 1280.0f;
		float screenY = (1.0f - ndc.y) * 0.5f * 720.0f; // 画面は下向きが+なので上下を反転
		aimDots_[i]->SetPosition(Vector2(screenX, screenY));

		// 狙っている方向は明るく大きく、それ以外は体力の減った分と同じ暗い色
		if (static_cast<int>(i) == litIndex) {
			aimDots_[i]->SetSize(Vector2(kAimDotLitSize, kAimDotLitSize));
			aimDots_[i]->SetColor(Vector4(0.3f, 0.85f, 1.0f, 1.0f));
		} else {
			aimDots_[i]->SetSize(Vector2(kAimDotSize, kAimDotSize));
			aimDots_[i]->SetColor(Vector4(0.2f, 0.2f, 0.25f, 0.6f));
		}
	}
}

void GamePlayScene::DrawAimMarker() {
	if (!isAimVisible_) {
		return;
	}
	for (Sprite* dot : aimDots_) {
		dot->Draw();
	}
}

void GamePlayScene::UpdateHud() {
	// ボス：残り体力の割合だけバーを伸ばす
	int bossHP = std::clamp(bossEnemy_->GetHP(), 0, kBossMaxHP);
	float rate = static_cast<float>(bossHP) / static_cast<float>(kBossMaxHP);
	bossHpFill_->SetSize(Vector2(kBossBarWidth * rate, kBossBarHeight));

	// プレイヤー：残っている分は明るく、減った分は暗くする
	int playerHP = std::clamp(player_->status_.HP, 0, kPlayerMaxHP);
	for (int i = 0; i < kPlayerMaxHP; ++i) {
		if (i < playerHP) {
			playerHpIcons_[i]->SetColor(Vector4(0.3f, 0.85f, 1.0f, 1.0f));
		} else {
			playerHpIcons_[i]->SetColor(Vector4(0.2f, 0.2f, 0.25f, 0.6f));
		}
	}
}

void GamePlayScene::DrawHud() {
	Sprite::PreDraw();
	DrawAimMarker();
	bossHpBack_->Draw();
	bossHpFill_->Draw();
	for (Sprite* icon : playerHpIcons_) {
		icon->Draw();
	}
	controlsSprite_->Draw();
	Sprite::PostDraw();
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

	// 体力のUI（3Dの上、フェードの下）
	DrawHud();

	fade_->Draw();
}

GamePlayScene::~GamePlayScene() {
	// シーンが終わるときにBGMを止める（止めないと次のシーンの音と重なる）
	KamataEngine::Audio::GetInstance()->StopWave(bgmVoice_);
	delete fade_;
	delete player_;
	delete modelAttack_;
	delete modelPlayer_;
	delete debugCamera_;
	delete skyDome_;
	delete modelBoss_;
	delete deathParticles_;

	// 体力のUI
	delete bossHpBack_;
	delete bossHpFill_;
	delete controlsSprite_;
	for (Sprite* icon : playerHpIcons_) {
		delete icon;
	}
	for (Sprite* dot : aimDots_) {
		delete dot;
	}

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

	if (bossEnemy_->isDeth()) {
		phese_ = Phese::kClear;
		deathParticles_->Initialize(modelBoss_, &cameraController_->GetCamera(), player_->GetWorldPodition());
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
