#pragma once

// =====================================================
// プレイヤー
//
// 座標は「画面上の座標」(左上が原点、下がプラス)。
// x_, y_ は当たり判定の箱(size_ × size_)の左上。
// 背景のスクロールは SceneManager 側が、GetY / SetY を使って行う。
//
// 動き:
//   スペースを「押した瞬間」に連打ゲージが溜まり、ゲージが多いほど速く登る。
//   登り終わると少し止まり(しがみつく)、そのあとずり落ちる。
// =====================================================
class Player {
public:
	// 画像の読み込み(Novice::Initialize の後に、1回だけ呼ぶ)
	void Initialize();

	// 初期位置・状態に戻す(ステージ開始・リトライのときに呼ぶ)
	void Reset();

	// 更新(毎フレーム呼ぶ。プレイ中のときだけ)
	// 「押した瞬間」を見るため、前フレームのキー状態も受け取る
	void Update(const char* keys, const char* preKeys);

	// 描画(キャラクター)
	void Draw() const;

	// 描画(連打ゲージ)
	void DrawGauge() const;

	// ---- 座標の取得・設定(スクロール処理や当たり判定で使う) ----
	float GetX() const { return x_; }
	float GetY() const { return y_; }
	float GetSize() const { return size_; }
	void SetY(float y) { y_ = y; }

	// 岩などに当たったときの硬直を始める。
	// 登る勢いが止まり、しがみつかずにすぐずり落ち始める。硬直中はスペースを受け付けない
	void Stun();
	bool IsStunned() const { return stunTimer_ > 0; }

	// 当たり判定(円)用。中心の座標と半径
	float GetCenterX() const { return x_ + size_ * 0.5f; }
	float GetCenterY() const { return y_ + size_ * 0.5f; }
	float GetRadius() const { return radius_; }

	// 速さ(1フレームで動く距離。上に登る向きが正、ずり落ちは負、止まっていれば0)
	// 突風や氷のロープの判定に使う
	float GetSpeed() const;

	// 連打ゲージ(0.0 〜 1.0)
	float GetMashGauge() const { return mashGauge_; }

private:
	// 左上の座標
	float x_ = 0.0f;
	float y_ = 0.0f;

	// 当たり判定の箱の大きさ
	float size_ = 40.0f;
	// 当たり判定の円の半径(落石など、円同士の判定で使う)
	float radius_ = 16.0f;

	// ---- 連打・登り・ずり落ちの状態 ----
	float mashGauge_ = 0.0f;    // 連打ゲージ(0.0 〜 1.0)
	float climbVelocity_ = 0.0f; // 登る速さ(上向きはマイナス)
	int dropDelayTimer_ = 0;     // ずり落ち始めるまでの残りフレーム
	int stunTimer_ = 0;          // 硬直の残りフレーム
	float slideSpeed_ = 0.0f;    // ずり落ちる速さ(下向きはプラス)

	// ---- アニメーション ----
	int textureHandle_ = -1; // player.png(-1 は未読み込み)
	int animFrame_ = 0;      // 今のコマ(0 か 1)。スペースを押すたびに切り替わる
};
