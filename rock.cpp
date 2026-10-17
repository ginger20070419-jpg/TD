#include "rock.h"
#include <Novice.h>

namespace {
const int kScreenWidth = 1280;
const int kScreenHeight = 1080;

// 岩が現れる・消える位置の余白(画面の上下の外側へ、このぶんだけ離れた場所)
const float kEdgeMargin = 60.0f;

// rock.png の大きさ(64×64。画像を作り直したらここも変える)
const float kRockImageSize = 64.0f;
} // namespace

void RockFall::Setup(const Params& params) {
	params_ = params;
	Reset();
}

void RockFall::Reset() {
	active_ = false;
	warning_ = false;
	// 最初の待ち時間をずらしたいときは、マイナスから数え始める
	timer_ = -params_.startOffsetFrames;
	worldY_ = 0.0f;
}

void RockFall::Update(float scrollY) {
	// 岩が落ちていない間は、時間を数える
	if (!active_) {
		timer_++;

		if (timer_ >= params_.warningStartFrames && timer_ < params_.spawnIntervalFrames) {
			// 警告を出して知らせる
			warning_ = true;
		} else if (timer_ >= params_.spawnIntervalFrames) {
			// 警告を消して、画面の上の外から岩を出す
			warning_ = false;
			active_ = true;
			worldY_ = -scrollY - kEdgeMargin;
			timer_ = 0;
		}
	}

	if (active_) {
		worldY_ += params_.speed; // 下へ落ちる

		// 画面の下の外へ出たら消す
		if (worldY_ + scrollY > kScreenHeight + kEdgeMargin) {
			active_ = false;
		}
	}
}

void RockFall::DrawWarning() const {
	if (!warning_) {
		return;
	}

	// 10フレームごとに点滅する
	if ((timer_ / 10) % 2 == 0) {
		Novice::DrawBox(0, 20, kScreenWidth, 40, 0.0f, 0xCC0000AA, kFillModeSolid);
		Novice::ScreenPrintf(static_cast<int>(params_.x) - 52, 32, "!! WARNING !!");
	}
}

void RockFall::Draw(float scrollY, int textureHandle) const {
	if (!active_) {
		return;
	}

	int screenX = static_cast<int>(params_.x);
	int screenY = static_cast<int>(worldY_ + scrollY);

	if (textureHandle >= 0) {
		// 画像の中心を、岩の中心に合わせて描く
		int half = static_cast<int>(kRockImageSize * 0.5f);
		Novice::DrawSprite(screenX - half, screenY - half, textureHandle, 1.0f, 1.0f, 0.0f, WHITE);
	} else {
		// 画像がないときは灰色の円で代用する
		Novice::DrawEllipse(
		    screenX, screenY, static_cast<int>(params_.radius), static_cast<int>(params_.radius), 0.0f, 0x888888FF,
		    kFillModeSolid);
	}
}

void RockFall::DrawDebug(float scrollY) const {
	if (!active_) {
		return;
	}
	Novice::DrawEllipse(
	    static_cast<int>(params_.x), static_cast<int>(worldY_ + scrollY), static_cast<int>(params_.radius),
	    static_cast<int>(params_.radius), 0.0f, 0xFF4444FF, kFillModeWireFrame);
}

bool RockFall::CheckHit(float playerCenterX, float playerCenterY, float playerRadius, float scrollY) const {
	if (!active_) {
		return false;
	}

	// 円と円の当たり判定(中心の距離が、半径の合計以下なら当たり)
	float dx = playerCenterX - params_.x;
	float dy = playerCenterY - (worldY_ + scrollY);
	float radiusSum = playerRadius + params_.radius;
	return dx * dx + dy * dy <= radiusSum * radiusSum;
}
