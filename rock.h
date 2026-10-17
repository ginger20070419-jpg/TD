#pragma once

// =====================================================
// 落石
//
// 待つ → 警告(点滅) → 岩が上から落ちてくる → 画面の下に消える → また待つ、を繰り返す。
// プレイヤーに当たったら岩は消え、プレイヤーは硬直する(硬直は Player::Stun)。
//
// このクラスは Player や SceneManager に依存しない。
// 「当たったかどうか」までを担当し、当たったあとの処理は呼び出し側で行う。
//
// 岩の位置は「ステージ座標」(下がプラス、上がマイナス)で持つ。
//   画面のY = ステージY + scrollY
// X は画面のX座標のまま(横には動かない)。
// =====================================================
class RockFall {
public:
	// ステージごとに変える設定値
	struct Params {
		float x = 640.0f;               // 落ちてくる位置(画面のX。岩の中心)
		float radius = 24.0f;           // 当たり判定の半径
		float speed = 9.0f;             // 落ちる速さ(1フレームで動く距離)
		int warningStartFrames = 120;   // 待ち始めてから、警告を出し始めるまで
		int spawnIntervalFrames = 180;  // 待ち始めてから、岩が出てくるまで
		int startOffsetFrames = 0;      // 最初の待ち時間を足す(複数の岩のタイミングをずらす)
	};

	// 設定して、最初の状態に戻す
	void Setup(const Params& params);

	// 最初の状態に戻す(ステージ開始・リトライのときに呼ぶ)
	void Reset();

	// 更新(毎フレーム。プレイ中のときだけ)
	void Update(float scrollY);

	// 描画(警告の帯。岩より先に呼ぶ)
	void DrawWarning() const;

	// 描画(岩。textureHandle は rock.png のハンドルで、-1 のときは灰色の円で代用)
	void Draw(float scrollY, int textureHandle) const;

	// 当たり判定の円の表示(デバッグ用)
	void DrawDebug(float scrollY) const;

	// プレイヤー(円)と当たっているか。プレイヤーの中心と半径は画面の座標で渡す
	bool CheckHit(float playerCenterX, float playerCenterY, float playerRadius, float scrollY) const;

	// 当たった岩を消す
	void Consume() { active_ = false; }

	bool IsActive() const { return active_; }
	bool IsWarning() const { return warning_; }

private:
	Params params_;
	bool active_ = false;  // 岩が落ちている最中か
	bool warning_ = false; // 警告を出している最中か
	int timer_ = 0;        // 待ち始めてからの経過フレーム
	float worldY_ = 0.0f;  // 岩の中心(ステージ座標)
};
