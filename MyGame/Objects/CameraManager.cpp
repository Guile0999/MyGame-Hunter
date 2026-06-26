#include "pch.h"
#include "CameraManager.h"
#include "Objects/FirstPersonPlayer.h"

namespace MyLib
{
    // プレイヤーの参照を渡す
    void CameraManager::Init(const FirstPersonPlayer* player)
    {
        m_player = player;
    }

    // FpsGun::Shoot()の直後に呼ぶ — 弾追従カメラへ切り替え
    void CameraManager::OnFire(Bullet* bullet)
    {
        m_liveBullet = bullet;
        m_bulletCam.OnFire(bullet);
        m_bulletCam.Update(bullet);   // 視点を即座に設定
        m_state = CamState::Bullet;
        m_freezeTimer = 0.0f;
    }

    // 弾が何かに命中した時に呼ぶ — 画面を一時停止
    void CameraManager::OnBulletHit()
    {
        m_state = CamState::Freeze;
        m_freezeTimer = 0.0f;
    }

    // 弾が範囲外/無効になった時に呼ぶ — 通常視点へ戻す
    void CameraManager::OnBulletLost()
    {
        m_bulletCam.Detach();
        m_state = CamState::Player;
    }

    // 毎フレーム呼ぶ。現在飛んでいる弾を渡す
    void CameraManager::Update(float dt, Bullet* liveBullet)
    {
        switch (m_state)
        {
        case CamState::Player:
            break;   // 通常視点はFirstPersonPlayer::Update()内で更新される

        case CamState::Bullet:
            // 弾が生きていれば追従、消えていれば通常視点へ
            if (liveBullet && liveBullet->IsActive())
                m_bulletCam.Update(liveBullet);
            else
                OnBulletLost();
            break;

        case CamState::Freeze:
            // 最後の弾カメラ映像を保持（Updateは呼ばない）
            m_freezeTimer += dt;
            if (m_freezeTimer >= freezeDuration)
                OnBulletLost();
            break;
        }
    }

    // 現在のアクティブなビュー行列を返す（状態に応じて切り替え）
    DirectX::SimpleMath::Matrix CameraManager::GetActiveView() const
    {
        if (m_state == CamState::Player)
        {
            // 通常視点：FirstPersonPlayer内のカメラを使う（シングルトンではない）
            return m_player->GetCamera().GetViewMatrix();
        }
        // 弾追従／停止：弾カメラのビューを使う
        return m_bulletCam.GetViewMatrix();
    }

    // 現在のアクティブなプロジェクション行列を返す
    DirectX::SimpleMath::Matrix CameraManager::GetActiveProjection(float aspectRatio) const
    {
        if (m_state == CamState::Player)
        {
            return m_player->GetCamera().GetProjectionMatrix(aspectRatio);
        }
        return m_bulletCam.GetProjectionMatrix(aspectRatio);
    }
}