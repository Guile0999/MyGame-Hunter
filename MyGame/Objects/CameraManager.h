// CameraManager.h
#pragma once
#include "pch.h"
#include "BulletCamera.h"
#include "Objects/Bullet.h"
#include "MyLib/Utilities/Singleton.h"

namespace MyLib
{
    // 前方宣言 — FirstPersonPlayerとの循環インクルードを避ける
    class FirstPersonPlayer;

    // カメラの状態：通常（プレイヤー）／弾追従／停止
    enum class CamState { Player, Bullet, Freeze };

    // カメラ管理クラス（シングルトン）：通常視点と弾追従カメラを切り替える
    class CameraManager : public Singleton<CameraManager>
    {
        friend class Singleton<CameraManager>;
    public:
        float freezeDuration = 0.5f;   // 命中後に画面を止める時間

        void Init(const FirstPersonPlayer* player);          // プレイヤーの参照を渡す
        void OnFire(Bullet* bullet);                         // 発射時：弾追従カメラへ切り替え
        void OnBulletHit();                                  // 命中時：画面を一時停止
        void OnBulletLost();                                 // 弾消失時：通常視点へ戻す
        void Update(float dt, Bullet* liveBullet = nullptr); // 毎フレーム更新

        // 描画側が呼ぶ — player.GetViewMatrix()の代わり
        DirectX::SimpleMath::Matrix GetActiveView() const;
        DirectX::SimpleMath::Matrix GetActiveProjection(float aspectRatio) const;
        CamState GetState() const { return m_state; }   // 現在のカメラ状態（簡単なゲッターはinline）

    private:
        CameraManager() = default;

        const FirstPersonPlayer* m_player = nullptr;  // 観測用の生ポインタ（所有しない）
        BulletCamera m_bulletCam;                     // 弾追従カメラ
        Bullet* m_liveBullet = nullptr;              // 現在追従中の弾（観測のみ）
        CamState m_state = CamState::Player;          // 現在の状態
        float    m_freezeTimer = 0.0f;                // 停止状態のタイマー
    };
}