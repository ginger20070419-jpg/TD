#pragma once

#include "player.h"
#include "wind.h"
#include <vector>

// =====================================================
//シーン管理
// =====================================================

// シーンの種類
enum class SceneType {
	kTitle, // タイトル
	kPlay,  // プレイ(クリア・ゲームオーバー表示中も含む)
};

// プレイシーンの中の状態
enum class PlayState {
	kPlaying,  // プレイ中
	kClear,    // クリア表示中(ゲームの更新は止める)
	kGameOver, // ゲームオーバー表示中(ゲームの更新は止める)
};

class SceneManager {
public:
	// ステージ数(企画では6ステージ)
	static const int kMaxStage = 6;

	// 初期化(画像の読み込みなど。Novice::Initialize の後に1回だけ呼ぶ)
	void Initialize();

	// 更新(毎フレーム呼ぶ)
	void Update(const char* keys, const char* preKeys);

	// 描画(毎フレーム呼ぶ)
	void Draw();

private:
	// ---- シーンごとの更新・描画 ----
	void UpdateTitle(const char* keys, const char* preKeys);
	void UpdatePlay(const char* keys, const char* preKeys);
	void DrawTitle();
	void DrawPlay();

	// ---- プレイ中の処理 ----
	// 上方向スクロール(プレイヤーが一定の高さを超えたら背景を動かす)
	void UpdateScroll();
	// 背景の描画(BG1を繰り返し、ゴール付近でBG2を流し込む)
	void DrawBackground();
	// 突風の更新・描画(ステージ座標への変換と、押す判定の呼び出しをここで行う)
	void UpdateWind();
	void DrawWind();

	// シーン切り替え
	void ChangeScene(SceneType next);

	// プレイを最初からやり直す(リトライ・次ステージ共通の初期化)
	void ResetPlay();

	// 「押した瞬間」だけtrueを返す
	bool IsTrigger(const char* keys, const char* preKeys, int key) const;

private:
	SceneType currentScene_ = SceneType::kTitle;
	PlayState playState_ = PlayState::kPlaying;

	// 現在のステージ番号(1〜kMaxStage)
	int currentStage_ = 1;

	// ---- スクロール ----
	// これまでにスクロールした距離(上に登るほど増える)
	float scrollY_ = 0.0f;
	// このステージで最大何px スクロールするか(= ステージの高さ)
	float stageScrollLength_ = 3000.0f;

	// ---- プレイヤー(player.h / player.cpp) ----
	Player player_;

	// ---- 突風(wind.h / wind.cpp) ----
	// ステージのデータ(Stage クラス)ができたら、そちらへ移す
	std::vector<WindZone> winds_;
	// 前のフレームのプレイヤーの高さ(ステージ座標)。速さの計算に使う
	float prevPlayerWorldY_ = 0.0f;
	// 【仮】押された向きの表示用(プレイヤーにレーンができたら不要)
	int windDebugDir_ = 0;
	int windDebugTimer_ = 0;

	// ---- 画像ハンドル(-1 は未読み込み) ----
	int titleBgHandle_ = -1;  // タイトル背景(素材ができたら読み込む)
	int playBg1Handle_ = -1;  // BG1
	int playBg2Handle_ = -1;  // BG2(ゴール付近用)
	int clearHandle_ = -1;    // gameclear.png
	int gameOverHandle_ = -1; // gameover.png
	// 突風の画像(3480×240)。名前は「吹いていく向き」で付けている
	int windToRightHandle_ = -1; // 左から右へ吹く風(windLeft.png)
	int windToLeftHandle_ = -1;  // 右から左へ吹く風(windRight.png)
};
