#include "player.h"
#include <Novice.h>

// -----------------------------------------------------
// 調整用の数値
// -----------------------------------------------------
namespace {
const int kScreenWidth = 1280;

const float kStartY = 800.0f; // 開始位置(画面のY座標)
const float kStartX = 620.0f; // 開始位置(画面のX座標)
const float kSpeed = 5.0f;    // 【仮】スペースを押している間、1フレームで登る距離
} // namespace

void Player::Reset() {
	x_ = kStartX;
	y_ = kStartY;
}

void Player::Update(const char* keys) {
	// 【仮】スペースを押している間、上に登る
	// 本実装(連打で加速・離すとずり落ちる)は、ここを書き換える
	if (keys[DIK_SPACE]) {
		y_ -= kSpeed; // 画面は下がプラスなので、上に登るにはマイナス
	}
}

void Player::Draw() const {
	// 【仮】白いボックス
	Novice::DrawBox(
	    static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(size_),
	    static_cast<int>(size_), 0.0f, 0xFFFFFFFF, kFillModeSolid);
}
