#include "pch.h"
#include "BulletCamera.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    // 発射時：追従する弾をセット
    void BulletCamera::OnFire(Bullet* bullet)
    {
        m_bullet = bullet;
        m_hasBullet = true;
    }

    // 弾を手放す（追従終了）
    void BulletCamera::Detach()
    {
        m_bullet = nullptr;
        m_hasBullet = false;
    }

    // 弾がまだ生きている（飛んでいる）か
    bool BulletCamera::BulletAlive() const
    {
        return m_hasBullet && m_bullet && m_bullet->IsActive();
    }

    // 更新：弾の位置・向きからカメラの視点を計算
    void BulletCamera::Update(Bullet* bullet)
    {
        if (!m_hasBullet || !bullet) return;

        // 弾の現在位置と進行方向を取得
        Vector3 pos = bullet->GetRay().origin;
        Vector3 dir = bullet->GetRay().direction;
        if (dir.LengthSquared() < 0.0001f) return;   // 方向が無効なら何もしない
        dir.Normalize();

        Vector3 up = Vector3::Up;
        // カメラ位置：弾の後方＋少し上
        Vector3 camPos = pos - dir * trailDistance + up * heightOffset;
        // 注視点：弾の少し前方
        Vector3 lookAt = pos + dir * 1.0f;
        m_view = Matrix::CreateLookAt(camPos, lookAt, up);
    }

    // プロジェクション行列を計算（視野角60度の透視投影）
    Matrix BulletCamera::GetProjectionMatrix(float aspectRatio) const
    {
        return Matrix::CreatePerspectiveFieldOfView(
            XMConvertToRadians(60.0f),   // 視野角（FOV）：60度
            aspectRatio,                 // アスペクト比
            0.01f,                       // ニアクリップ
            2000.0f                      // ファークリップ
        );
    }
}