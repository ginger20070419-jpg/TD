#include "scene.h"
#include <Novice.h>
#include <cmath>

// -----------------------------------------------------
// 調整用の数値(ここをいじるだけで挙動を変えられる)
// -----------------------------------------------------
namespace {
// 画面サイズ
const int kScreenWidth = 1280;
const int kScreenHeight = 1080;

// 背景画像の高さ(BG1・BG2とも1080px。)
const float kBgHeight = 1080.0f;

// この高さ(画面のY座標)より上にプレイヤーが行こうとしたらスクロールを始める
const float kScrollLineY = 360.0f;

// スクロールが終わったあと、この高さまで登ったらゴール(画面のY座標)
const float kGoalY = 250.0f;

// 【仮】デバッグ用。突風のテスト用エリアを置き、範囲と状態を表示する。
// ステージのデータができたら false にする(または消す)
const bool kDebugWind = true;

// 【仮】デバッグ用。落石のテスト用の岩を置き、当たり判定の円を表示する。
// ステージのデータができたら false にする(または消す)
const bool kDebugRock = true;
} // namespace

// -----------------------------------------------------
// 初期化
// -----------------------------------------------------
void SceneManager::Initialize() {
	titleBgHandle_ = Novice::LoadTexture("./Resources/images/title.png");

	playBg1Handle_ = Novice::LoadTexture("./Resources/images/BG1.png");
	playBg2Handle_ = Novice::LoadTexture("./Resources/images/BG2.png");
	clearHandle_ = Novice::LoadTexture("./Resources/images/gameclear.png");
	gameOverHandle_ = Novice::LoadTexture("./Resources/images/gameover.png");
	// ファイル名は「風が来る側」、変数名は「吹いていく向き」。
	//   windLeft.png  = 左から来て右へ吹く風
	//   windRight.png = 右から来て左へ吹く風
	windToRightHandle_ = Novice::LoadTexture("./Resources/images/windLeft.png");
	windToLeftHandle_ = Novice::LoadTexture("./Resources/images/windRight.png");

	// 落石の画像
	rockHandle_ = Novice::LoadTexture("./Resources/images/rock.png");

	// プレイヤーの画像
	player_.Initialize();

	currentScene_ = SceneType::kTitle;
	playState_ = PlayState::kPlaying;
	currentStage_ = 1;
	exitRequested_ = false;
}

// -----------------------------------------------------
// 更新・描画(現在のシーンに振り分ける)
// -----------------------------------------------------
void SceneManager::Update(const char* keys, const char* preKeys) {
	switch (currentScene_) {
	case SceneType::kTitle:
		UpdateTitle(keys, preKeys);
		break;
	case SceneType::kPlay:
		UpdatePlay(keys, preKeys);
		break;
	}
}

void SceneManager::Draw() {
	switch (currentScene_) {
	case SceneType::kTitle:
		DrawTitle();
		break;
	case SceneType::kPlay:
		DrawPlay();
		break;
	}
}

// -----------------------------------------------------
// タイトル
// -----------------------------------------------------
void SceneManager::UpdateTitle(const char* keys, const char* preKeys) {
	// Pキーでゲーム開始(ステージ1から)
	if (IsTrigger(keys, preKeys, DIK_P)) {
		currentStage_ = 1;
		ChangeScene(SceneType::kPlay);
	}

	// Eキーで終了(main のループが ShouldExit を見て抜ける)
	if (IsTrigger(keys, preKeys, DIK_E)) {
		exitRequested_ = true;
	}
}

void SceneManager::DrawTitle() {
	if (titleBgHandle_ >= 0) {
		// 素材ができたらこちらが使われる
		Novice::DrawSprite(0, 0, titleBgHandle_, 1.0f, 1.0f, 0.0f, WHITE);
	} else {
		// 素材がない間の仮表示
		Novice::DrawBox(0, 0, kScreenWidth, kScreenHeight, 0.0f, 0x202020FF, kFillModeSolid);
		Novice::ScreenPrintf(540, 300, "NOBORENAIGHTS (title)");
		Novice::ScreenPrintf(540, 340, "P: START   E: EXIT");
	}
}

// -----------------------------------------------------
// プレイ(クリア・ゲームオーバーの表示もここで行う)
// -----------------------------------------------------
void SceneManager::UpdatePlay(const char* keys, const char* preKeys) {
	switch (playState_) {
	case PlayState::kPlaying:
		// ゲームの更新は「プレイ中」のときだけ動かす
		player_.Update(keys, preKeys); // プレイヤーの移動(連打・しがみつき・ずり落ち)
		UpdateScroll();       // スクロール
		UpdateWind();         // 突風(スクロール後のプレイヤー位置で判定する)
		UpdateRock();         // 落石(当たったらプレイヤーが硬直する)

		// ゴール判定(スクロールが終わったあと、一定の高さまで登ったらクリア)
		if (scrollY_ >= stageScrollLength_ && player_.GetY() <= kGoalY) {
			playState_ = PlayState::kClear;
		}

		// 本来の遷移条件
		//   棘やマグマに接触 → playState_ = PlayState::kGameOver;

		// 【仮】動作確認用のデバッグキー。完成したら消す
		if (IsTrigger(keys, preKeys, DIK_C)) {
			playState_ = PlayState::kClear;
		}
		if (IsTrigger(keys, preKeys, DIK_O)) {
			playState_ = PlayState::kGameOver;
		}
		break;

	case PlayState::kClear:
		// Nキー: 次のステージへ(最終ステージならタイトルへ戻る)
		if (IsTrigger(keys, preKeys, DIK_N)) {
			if (currentStage_ < kMaxStage) {
				currentStage_++;
				ResetPlay();
			} else {
				currentStage_ = 1;
				ChangeScene(SceneType::kTitle);
			}
		}
		break;

	case PlayState::kGameOver:
		// Rキー: 同じステージをやり直す
		if (IsTrigger(keys, preKeys, DIK_R)) {
			ResetPlay();
		}
		// Tキー: タイトルへ戻る
		if (IsTrigger(keys, preKeys, DIK_T)) {
			currentStage_ = 1;
			ChangeScene(SceneType::kTitle);
		}
		break;
	}
}

void SceneManager::DrawPlay() {
	// ---- プレイ画面(状態に関係なく常に描く。止まった背景として見える) ----
	DrawBackground();

	// 落石(警告 → 岩。プレイヤーより奥に描く)
	DrawRock();

	// プレイヤー
	player_.Draw();

	// 突風(プレイヤーより手前に描く)
	DrawWind();

	// 連打ゲージ
	player_.DrawGauge();

	// ここにロープ・ギミックの描画を書いていく

	Novice::ScreenPrintf(20, 20, "PLAY  STAGE %d / %d", currentStage_, kMaxStage);
	Novice::ScreenPrintf(20, 40, "SPACE: climb   [debug] C: clear  O: game over");
	Novice::ScreenPrintf(20, 60, "scrollY: %.0f / %.0f   playerY: %.0f", scrollY_, stageScrollLength_, player_.GetY());

	// ---- その上に重ねて表示 ----
	if (playState_ == PlayState::kClear) {
		Novice::DrawSprite(0, 0, clearHandle_, 1.0f, 1.0f, 0.0f, WHITE);
	} else if (playState_ == PlayState::kGameOver) {
		Novice::DrawSprite(0, 0, gameOverHandle_, 1.0f, 1.0f, 0.0f, WHITE);
	}
}

// -----------------------------------------------------
// 上方向スクロール
// プレイヤーがスクロールラインより上に行こうとしたら、
// プレイヤーはラインに留めて、その分だけ背景を動かす。
// -----------------------------------------------------
void SceneManager::UpdateScroll() {
	// すでにスクロールが終わっている(ゴール付近)なら何もしない
	if (scrollY_ >= stageScrollLength_) {
		return;
	}

	float playerY = player_.GetY();

	if (playerY < kScrollLineY) {
		// ラインを越えた分
		float over = kScrollLineY - playerY;

		// プレイヤーはラインに戻して、越えた分を背景のスクロール量にする
		playerY = kScrollLineY;
		scrollY_ += over;

		// 最大スクロール量を超えたら、超えた分はプレイヤーの移動に戻す
		if (scrollY_ > stageScrollLength_) {
			float extra = scrollY_ - stageScrollLength_;
			scrollY_ = stageScrollLength_;
			playerY = kScrollLineY - extra;
		}

		player_.SetY(playerY);
	}
}

// -----------------------------------------------------
// 背景の描画
// -----------------------------------------------------
void SceneManager::DrawBackground() {
	// ---- BG1: 縦に繰り返す ----
	// scrollY_ を画像の高さで割った余りぶん、下にずらして描く。
	// 上下に1枚ずつ並べれば、隙間なく流れ続ける。
	float offset = std::fmod(scrollY_, kBgHeight);
	if (playBg1Handle_ >= 0) {
		Novice::DrawSprite(0, static_cast<int>(offset - kBgHeight), playBg1Handle_, 1.0f, 1.0f, 0.0f, WHITE);
		Novice::DrawSprite(0, static_cast<int>(offset), playBg1Handle_, 1.0f, 1.0f, 0.0f, WHITE);
	}

	// ---- BG2: ステージの一番上に置く ----
	// 最大までスクロールしたとき、BG2の上端が画面の一番上(Y=0)に来る。
	// それより手前では画面の上の外側にいて、スクロールが進むと上から降りてくる。
	float bg2Y = scrollY_ - stageScrollLength_;
	if (playBg2Handle_ >= 0 && bg2Y > -kBgHeight) {
		Novice::DrawSprite(0, static_cast<int>(bg2Y), playBg2Handle_, 1.0f, 1.0f, 0.0f, WHITE);
	}
}

// -----------------------------------------------------
// 突風
// -----------------------------------------------------
void SceneManager::UpdateWind() {
	// プレイヤーの位置を、スクロールに影響されない「ステージ座標」にする
	float playerTop = player_.GetY() - scrollY_;
	float playerBottom = playerTop + player_.GetSize();

	// 速さ(上に登ると正、ずり落ちは負、しがみつき中は0)
	float speed = player_.GetSpeed();

	for (WindZone& wind : winds_) {
		wind.Update();

		int dir = wind.CheckPush(playerTop, playerBottom, speed);
		if (dir != 0) {
			// ここでプレイヤーを隣のロープへ移動させる。
			// プレイヤーにレーンができたら、例: player_.SetLane(player_.GetLane() + dir);
			windDebugDir_ = dir;
			windDebugTimer_ = 90;
		}
	}

	if (windDebugTimer_ > 0) {
		windDebugTimer_--;
	}
}

void SceneManager::DrawWind() {
	for (const WindZone& wind : winds_) {
		// 風の向きに合った画像を渡す
		int handle = (wind.GetParams().direction > 0) ? windToRightHandle_ : windToLeftHandle_;
		wind.Draw(scrollY_, handle);
		if (kDebugWind) {
			wind.DrawDebug(scrollY_);
		}
	}

	if (kDebugWind && windDebugTimer_ > 0) {
		Novice::ScreenPrintf(20, 80, "WIND PUSH -> %s", windDebugDir_ > 0 ? "RIGHT" : "LEFT");
	}
}

// -----------------------------------------------------
// 落石
// -----------------------------------------------------
void SceneManager::UpdateRock() {
	for (RockFall& rock : rocks_) {
		rock.Update(scrollY_);

		// すでに硬直中なら、重ねて当たったことにはしない(岩もそのまま通り過ぎる)
		if (!player_.IsStunned() &&
		    rock.CheckHit(player_.GetCenterX(), player_.GetCenterY(), player_.GetRadius(), scrollY_)) {
			player_.Stun(); // プレイヤーが硬直して落ちる
			rock.Consume(); // 当たった岩は消える
		}
	}
}

void SceneManager::DrawRock() {
	for (const RockFall& rock : rocks_) {
		rock.DrawWarning();
		rock.Draw(scrollY_, rockHandle_);
		if (kDebugRock) {
			rock.DrawDebug(scrollY_);
		}
	}
}

// -----------------------------------------------------
// シーン切り替え・補助関数
// -----------------------------------------------------
void SceneManager::ChangeScene(SceneType next) {
	currentScene_ = next;

	// プレイに入るときは必ず初期状態に戻す
	if (next == SceneType::kPlay) {
		ResetPlay();
	}
}

void SceneManager::ResetPlay() {
	// 状態を「プレイ中」に戻す(これを忘れるとクリア表示のままになる)
	playState_ = PlayState::kPlaying;

	// スクロールとプレイヤーを初期位置に戻す
	scrollY_ = 0.0f;
	player_.Reset();

	// 突風を初期状態に戻す
	windDebugDir_ = 0;
	windDebugTimer_ = 0;
	winds_.clear();
	if (kDebugWind) {
		// 【仮】テスト用の突風を置く(ステージ4のデータができたらそちらへ移す)
		WindZone::Params p;
		p.worldY = -600.0f; // 上端(ステージ座標)。画面の高さ800からだと、約1400px登った先
		p.height = 240.0f;
		p.direction = 1; // 右へ
		winds_.emplace_back();
		winds_.back().Setup(p);

		// 左へ吹く風もテストできるように、もう1つ置く(タイミングをずらしてある)
		WindZone::Params q;
		q.worldY = -1100.0f;
		q.height = 240.0f;
		q.direction = -1; // 左へ
		q.startOffsetFrames = 60;
		winds_.emplace_back();
		winds_.back().Setup(q);
	}

	// 落石を初期状態に戻す
	rocks_.clear();
	if (kDebugRock) {
		// 【仮】テスト用の岩を1つ置く(ステージ4のデータができたらそちらへ移す)
		RockFall::Params r;
		r.x = 640.0f; // 画面中央のロープの位置
		rocks_.emplace_back();
		rocks_.back().Setup(r);
	}

	// ステージごとの高さを変えたくなったらここで currentStage_ に応じて設定する
	// 例: stageScrollLength_ = 3000.0f + currentStage_ * 500.0f;

	// currentStage_ に合わせてギミック配置・マグマ位置などを初期化する
	// 例: LoadStage(currentStage_);
}

bool SceneManager::IsTrigger(const char* keys, const char* preKeys, int key) const {
	return keys[key] != 0 && preKeys[key] == 0;
}
