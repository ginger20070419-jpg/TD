#pragma once

// =====================================================
// 突風エリア
//
// 一定の高さの帯(エリア)で、時間が来ると横から風が吹く。
//   待機(クールタイム) → 予告 → 吹いている → 待機 …… を繰り返す。
// 吹いている間にエリアの中で一定以上の速さだと、風の向きのロープへ押される。
//
// このクラスは Player や SceneManager に依存しない。
// 「押すかどうかの判定」までを担当し、実際にロープ(レーン)を動かす処理は
// 呼び出し側(プレイヤーのレーン実装ができたあと)で行う。
//
// 座標は「ステージ座標」。スクロール0のときの画面と同じ向きで、
// 下がプラス、ステージの上のほうはマイナスになる。
//   画面のY = ステージY + scrollY
// =====================================================
class WindZone {
public:
	// 動き方
	enum class Phase {
		kIdle,    // 吹いていない(クールタイム)
		kWarning, // 予告(まもなく吹く)
		kBlowing, // 吹いている
	};

	// ステージごとに変える設定値
	struct Params {
		float worldY = 0.0f;       // エリアの上端(ステージ座標。上に行くほどマイナス)
		float height = 240.0f;     // エリアの高さ(wind.png の高さ240pxなら等倍)
		int direction = 1;         // -1: 左へ吹く / +1: 右へ吹く
		float minSpeed = 3.0f;     // この速さ以上だと押される(1フレームで登る距離px)
		int idleFrames = 180;      // 吹き終わってから次の予告までの時間(クールタイム)
		int warningFrames = 40;    // 予告の長さ
		int blowFrames = 120;      // 吹いている長さ
		int startOffsetFrames = 0; // 最初の待ち時間を足す(複数のエリアのタイミングをずらす)
	};

	// 設定して、タイミングを最初に戻す
	void Setup(const Params& params);

	// タイミングを最初に戻す(ステージ開始・リトライのときに呼ぶ)
	void Reset();

	// 更新(毎フレーム。プレイ中のときだけ)
	void Update();

	// 描画(textureHandle は風の向きに合った画像のハンドル。-1 のときは仮の帯を表示)
	void Draw(float scrollY, int textureHandle) const;

	// 範囲と状態の表示(デバッグ用)
	void DrawDebug(float scrollY) const;

	// 押す判定。
	//   playerTop / playerBottom: プレイヤーの上端・下端(ステージ座標)
	//   playerSpeed: 1フレームで登った距離(上に登ると正の値)
	// 戻り値: 0=押されない / -1=左へ / +1=右へ
	// 1回の突風につき1回だけ押す(吹いている間ずっと押し続けない)。
	int CheckPush(float playerTop, float playerBottom, float playerSpeed);

	Phase GetPhase() const { return phase_; }
	int GetTimer() const { return timer_; }
	const Params& GetParams() const { return params_; }

private:
	Params params_;
	Phase phase_ = Phase::kIdle;
	int timer_ = 0;       // 今の動きがあと何フレームで終わるか
	bool pushed_ = false; // この突風でもう押したか
};
