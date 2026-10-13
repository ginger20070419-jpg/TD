#include "scene.h"
#include <Novice.h>
#include <cmath>

// -----------------------------------------------------
// 調整用の数値(ここをいじるだけで挙動を変えられる)
// -----------------------------------------------------
namespace {
// 画面サイズ
const int kScreenWidth = 1280;
const int kScreenHeight = 720;

// 背景画像の高さ(BG1・BG2とも1080px。)
const float kBgHeight = 1080.0f;

// この高さ(画面のY座標)より上にプレイヤーが行こうとしたらスクロールを始める
const float kScrollLineY = 360.0f;

// スクロールが終わったあと、この高さまで登ったらゴール(画面のY座標)
const float kGoalY = 250.0f;
} // namespace

// -----------------------------------------------------
// 初期化
// -----------------------------------------------------
void SceneManager::Initialize() {
	// タイトル画像はまだないので読み込まない(-1のまま)
	// 素材ができたら下の行のコメントを外してファイル名を合わせる
	// titleBgHandle_ = Novice::LoadTexture("./Resources/images/title.png");

	playBg1Handle_ = Novice::LoadTexture("./Resources/images/BG1.png");
	playBg2Handle_ = Novice::LoadTexture("./Resources/images/BG2.png");
	clearHandle_ = Novice::LoadTexture("./Resources/images/gameclear.png");
	gameOverHandle_ = Novice::LoadTexture("./Resources/images/gameover.png");

	currentScene_ = SceneType::kTitle;
	playState_ = PlayState::kPlaying;
	currentStage_ = 1;
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
	// スペースでゲーム開始(ステージ1から)
	if (IsTrigger(keys, preKeys, DIK_SPACE)) {
		currentStage_ = 1;
		ChangeScene(SceneType::kPlay);
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
		Novice::ScreenPrintf(540, 340, "PUSH SPACE TO START");
	}
}

// -----------------------------------------------------
// プレイ(クリア・ゲームオーバーの表示もここで行う)
// -----------------------------------------------------
void SceneManager::UpdatePlay(const char* keys, const char* preKeys) {
	switch (playState_) {
	case PlayState::kPlaying:
		// ゲームの更新は「プレイ中」のときだけ動かす
		player_.Update(keys); // プレイヤーの移動
		UpdateScroll();       // スクロール

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

	// プレイヤー
	player_.Draw();

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

	// ステージごとの高さを変えたくなったらここで currentStage_ に応じて設定する
	// 例: stageScrollLength_ = 3000.0f + currentStage_ * 500.0f;

	// currentStage_ に合わせてギミック配置・マグマ位置などを初期化する
	// 例: LoadStage(currentStage_);
}

bool SceneManager::IsTrigger(const char* keys, const char* preKeys, int key) const {
	return keys[key] != 0 && preKeys[key] == 0;
}
