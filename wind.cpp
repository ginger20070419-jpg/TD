#include "wind.h"
#include <Novice.h>
#include <algorithm>

// 注意: Windows.h が min / max というマクロを定義しているため、
// std::min や std::max は (std::min)(...) のようにカッコで囲んで書く。

namespace {
	const int kScreenWidth = 1280;
	const int kScreenHeight = 1080;

	// wind.png の大きさ(画像を作り直したらここも変える)
	const float kWindImageWidth = 3480.0f;
	const float kWindImageHeight = 240.0f;

	// 吹き始めと吹き終わりに、薄くなっていく割合(0.1 = 全体の10%ぶん)
	const float kFadeRate = 0.1f;
} // namespace

void WindZone::Setup(const Params& params) {
	params_ = params;
	Reset();
}

void WindZone::Reset() {
	phase_ = Phase::kIdle;
	timer_ = params_.idleFrames + params_.startOffsetFrames;
	pushed_ = false;
}

void WindZone::Update() {
	timer_--;
	if (timer_ > 0) {
		return;
	}

	// 時間が来たら、次の動きに進む
	switch (phase_) {
	case Phase::kIdle:
		phase_ = Phase::kWarning;
		timer_ = params_.warningFrames;
		break;
	case Phase::kWarning:
		phase_ = Phase::kBlowing;
		timer_ = params_.blowFrames;
		pushed_ = false; // 新しい突風が始まるので、押す権利をリセット
		break;
	case Phase::kBlowing:
		phase_ = Phase::kIdle;
		timer_ = params_.idleFrames;
		break;
	}
}

int WindZone::CheckPush(float playerTop, float playerBottom, float playerSpeed) {
	if (phase_ != Phase::kBlowing || pushed_) {
		return 0;
	}

	// プレイヤーがエリアの中にいるか(上下が少しでも重なっていればOK)
	bool inZone = playerBottom > params_.worldY && playerTop < params_.worldY + params_.height;
	if (!inZone) {
		return 0;
	}

	// 速ければ押される。遅ければ、しがみついて耐える
	if (playerSpeed >= params_.minSpeed) {
		pushed_ = true;
		return params_.direction;
	}
	return 0;
}

void WindZone::Draw(float scrollY, int textureHandle) const {
	float screenY = params_.worldY + scrollY;

	// 画面の外なら描かない
	if (screenY > kScreenHeight || screenY + params_.height < 0.0f) {
		return;
	}

	if (phase_ == Phase::kWarning) {
		// 予告: エリアを点滅させる(仮の表示。矢印などの素材ができたら差し替える)
		if ((timer_ / 4) % 2 == 0) {
			Novice::DrawBox(
				0, static_cast<int>(screenY), kScreenWidth, static_cast<int>(params_.height), 0.0f,
				0x88CCFF50, kFillModeSolid);
		}
		return;
	}

	if (phase_ == Phase::kBlowing) {
		// 画像がまだないときは、薄い帯で代わりに表示する(仮)
		if (textureHandle < 0) {
			Novice::DrawBox(
				0, static_cast<int>(screenY), kScreenWidth, static_cast<int>(params_.height), 0.0f,
				0xFFFFFF50, kFillModeSolid);
			return;
		}

		// 吹いている: 画像が画面を横切っていく。
		// 進み具合 t は 0(吹き始め)〜 1(吹き終わり)
		float t = 1.0f - static_cast<float>(timer_) / static_cast<float>((std::max)(params_.blowFrames, 1));
		t = std::clamp(t, 0.0f, 1.0f);

		float travel = kScreenWidth + kWindImageWidth; // 画像が動く距離
		float x = 0.0f;
		if (params_.direction > 0) {
			x = -kWindImageWidth + t * travel; // 左の外から右へ
		}
		else {
			x = kScreenWidth - t * travel; // 右の外から左へ
		}

		// 吹き始めと吹き終わりは薄くする
		float fade = (std::min)(t, 1.0f - t) / kFadeRate;
		fade = std::clamp(fade, 0.0f, 1.0f);
		unsigned int alpha = static_cast<unsigned int>(255.0f * fade);
		unsigned int color = 0xFFFFFF00 | alpha;

		float scaleY = params_.height / kWindImageHeight;
		Novice::DrawSprite(static_cast<int>(x), static_cast<int>(screenY), textureHandle, 1.0f, scaleY, 0.0f, color);
	}
}

void WindZone::DrawDebug(float scrollY) const {
	float screenY = params_.worldY + scrollY;
	if (screenY > kScreenHeight || screenY + params_.height < 0.0f) {
		return;
	}

	// 状態ごとに枠の色を変える(待機=水色 / 予告=黄色 / 吹いている=赤)
	unsigned int color = 0x66CCFFFF;
	if (phase_ == Phase::kWarning) {
		color = 0xFFFF00FF;
	}
	else if (phase_ == Phase::kBlowing) {
		color = 0xFF4444FF;
	}
	Novice::DrawBox(
		0, static_cast<int>(screenY), kScreenWidth, static_cast<int>(params_.height), 0.0f, color,
		kFillModeWireFrame);
	Novice::ScreenPrintf(
		kScreenWidth - 330, static_cast<int>(screenY) + 4, "WIND dir=%d phase=%d timer=%d", params_.direction,
		static_cast<int>(phase_), timer_);
}
