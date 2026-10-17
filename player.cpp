#include "player.h"
#include <Novice.h>

// -----------------------------------------------------
// 調整用の数値(ここをいじるだけで手触りを変えられる)
// -----------------------------------------------------
namespace {
// 画面サイズ(背景に合わせている)
const int kScreenWidth = 1280;
const int kScreenHeight = 1080;

// 開始位置(画面のX・Y座標。当たり判定の箱の左上。Xは画面中央のロープの位置)
const float kStartX = 620.0f;
const float kStartY = 800.0f;

// ---- 連打ゲージ ----
const float kMashAdd = 0.10f;    // スペース1回で溜まる量(10%)
const float kMashDecay = 0.008f; // 毎フレーム自然に減る量

// ---- 登る速さ(上向きはマイナス) ----
const float kMinClimbSpeed = -9.0f;  // ゲージが空のときの、1回の登る速さ
const float kMaxClimbSpeed = -24.0f; // ゲージが満タンのときの、1回の登る速さ
const float kClimbDamping = 0.85f;   // 登る速さが毎フレーム弱まる割合

// ---- ずり落ち ----
const int kDropDelayFrames = 120;  // 登り終わってから、ずり落ち始めるまで(しがみつく時間)
const float kSlideAccel = 0.60f;   // ずり落ちる速さが増える量(毎フレーム)
const float kMaxSlideSpeed = 8.0f; // ずり落ちる速さの上限

// ---- 硬直(岩に当たったとき) ----
const int kStunFrames = 60; // 硬直の長さ(60フレーム = 約1秒)

// ---- 画像(player.png: 横2コマ、1コマが 64×129、全体が 128×129) ----
const int kFrameWidth = 64;
const int kFrameHeight = 129;
const int kTextureWidth = 128;
const float kSpriteCenterX = 32.0f; // 1コマの中で、キャラの中心にあたる位置
const float kSpriteCenterY = 66.0f;
const float kDrawScale = 1.0f;      // 表示の大きさ(1.0 で画像そのままの大きさ)

// ---- 連打ゲージの表示(画面左の暗い帯の中) ----
const int kGaugeX = 20;
const int kGaugeY = 400;
const int kGaugeW = 40;
const int kGaugeH = 300;
} // namespace

void Player::Initialize() {
	textureHandle_ = Novice::LoadTexture("./Resources/images/player.png");
}

void Player::Reset() {
	x_ = kStartX;
	y_ = kStartY;

	mashGauge_ = 0.0f;
	climbVelocity_ = 0.0f;
	slideSpeed_ = 0.0f;
	// 開始直後にいきなりずり落ちないよう、最初もしがみつく時間から始める
	dropDelayTimer_ = kDropDelayFrames;
	animFrame_ = 0;
	stunTimer_ = 0;
}

void Player::Update(const char* keys, const char* preKeys) {
	// 硬直の残り時間を減らす
	if (stunTimer_ > 0) {
		stunTimer_--;
	}

	// 連打ゲージは、毎フレーム自然に減る
	mashGauge_ -= kMashDecay;
	if (mashGauge_ < 0.0f) {
		mashGauge_ = 0.0f;
	}

	// スペースを「押した瞬間」: ゲージが溜まり、ゲージの量に応じた速さで登る
	// (硬直中は受け付けない)
	if (stunTimer_ <= 0 && keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
		mashGauge_ += kMashAdd;
		if (mashGauge_ > 1.0f) {
			mashGauge_ = 1.0f;
		}

		climbVelocity_ = kMinClimbSpeed + (kMaxClimbSpeed - kMinClimbSpeed) * mashGauge_;
		dropDelayTimer_ = kDropDelayFrames;
		slideSpeed_ = 0.0f;

		// 押すたびに、手足が入れ替わる
		animFrame_ = 1 - animFrame_;
	}

	if (climbVelocity_ < -0.01f) {
		// 登っている(だんだん弱まる)
		y_ += climbVelocity_;
		climbVelocity_ *= kClimbDamping;
		if (climbVelocity_ >= -0.01f) {
			climbVelocity_ = 0.0f;
		}
	} else if (dropDelayTimer_ > 0) {
		// しがみついている
		dropDelayTimer_--;
	} else {
		// ずり落ちている(だんだん速くなる)
		slideSpeed_ += kSlideAccel;
		if (slideSpeed_ > kMaxSlideSpeed) {
			slideSpeed_ = kMaxSlideSpeed;
		}
		y_ += slideSpeed_;
	}

	// 画面の下まで落ちたら、そこで止まる
	float floorY = kScreenHeight - size_;
	if (y_ > floorY) {
		y_ = floorY;
		slideSpeed_ = 0.0f;
		climbVelocity_ = 0.0f;
	}
}

float Player::GetSpeed() const {
	if (climbVelocity_ < -0.01f) {
		return -climbVelocity_; // 登っている(プラス)
	}
	if (dropDelayTimer_ > 0) {
		return 0.0f; // しがみついている
	}
	return -slideSpeed_; // ずり落ちている(マイナス)
}

void Player::Stun() {
	stunTimer_ = kStunFrames;
	climbVelocity_ = 0.0f; // 登る勢いを止めて、落とす
	dropDelayTimer_ = 0;   // しがみつく時間を飛ばして、すぐずり落ち始める
}

void Player::Draw() const {
	// 硬直中は点滅させる(4フレームごとに消える)
	if (stunTimer_ > 0 && (stunTimer_ / 4) % 2 == 0) {
		return;
	}

	// 画像がないときは、白いボックスで代わりに表示する
	if (textureHandle_ < 0) {
		Novice::DrawBox(
		    static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(size_),
		    static_cast<int>(size_), 0.0f, 0xFFFFFFFF, kFillModeSolid);
		return;
	}

	// 当たり判定の箱の中心に、キャラの中心を合わせて描く
	float centerX = x_ + size_ * 0.5f;
	float centerY = y_ + size_ * 0.5f;
	int destX = static_cast<int>(centerX - kSpriteCenterX * kDrawScale);
	int destY = static_cast<int>(centerY - kSpriteCenterY * kDrawScale);

	// 横2コマのうち、今のコマだけを切り出して描く。
	// Novice の DrawSpriteRect は、拡大率が「画像全体」に対する割合になるため、
	// 横は 切り出し幅 ÷ 画像全体の幅 を掛けている。
	float scaleX = (static_cast<float>(kFrameWidth) / static_cast<float>(kTextureWidth)) * kDrawScale;
	float scaleY = kDrawScale;
	Novice::DrawSpriteRect(
	    destX, destY, animFrame_ * kFrameWidth, 0, kFrameWidth, kFrameHeight, textureHandle_, scaleX, scaleY, 0.0f,
	    WHITE);
}

void Player::DrawGauge() const {
	// 枠と背景
	Novice::DrawBox(kGaugeX, kGaugeY, kGaugeW, kGaugeH, 0.0f, 0x111111FF, kFillModeSolid);

	// 中身(下から溜まる。多いほど色が変わる)
	int fillH = static_cast<int>(kGaugeH * mashGauge_);
	int fillY = kGaugeY + kGaugeH - fillH;
	unsigned int color = 0x00FF88FF; // 緑
	if (mashGauge_ > 0.7f) {
		color = 0xFF3300FF; // 赤
	} else if (mashGauge_ > 0.35f) {
		color = 0xFFCC00FF; // 黄
	}
	if (fillH > 0) {
		Novice::DrawBox(kGaugeX, fillY, kGaugeW, fillH, 0.0f, color, kFillModeSolid);
	}
	Novice::DrawBox(kGaugeX, kGaugeY, kGaugeW, kGaugeH, 0.0f, 0x888888FF, kFillModeWireFrame);

	Novice::ScreenPrintf(kGaugeX, kGaugeY - 24, "GAUGE");
	Novice::ScreenPrintf(kGaugeX, kGaugeY + kGaugeH + 8, "%3d%%", static_cast<int>(mashGauge_ * 100.0f));
}
