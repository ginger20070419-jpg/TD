#pragma once

// =====================================================
// プレイヤー
//
// 座標は「画面上の座標」(左上が原点、下がプラス)。
// 背景のスクロールは SceneManager 側が、GetY / SetY を使って行う。
// =====================================================
class Player {
public:
	// 初期位置に戻す(ステージ開始・リトライのときに呼ぶ)
	void Reset();

	// 更新(毎フレーム呼ぶ。プレイ中のときだけ)
	void Update(const char* keys);

	// 描画
	void Draw() const;

	// ---- 座標の取得・設定(スクロール処理や当たり判定で使う) ----
	float GetX() const { return x_; }
	float GetY() const { return y_; }
	float GetSize() const { return size_; }
	void SetY(float y) { y_ = y; }

private:
	// 左上の座標
	float x_ = 0.0f;
	float y_ = 0.0f;

	// 【仮】白いボックスの大きさ
	float size_ = 40.0f;
};
