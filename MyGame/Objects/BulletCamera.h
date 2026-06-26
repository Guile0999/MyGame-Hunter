#pragma once
#include "pch.h"
#include <SimpleMath.h>
#include "Objects/Bullet.h"
#include <memory>

namespace MyLib
{
    // 弾追従カメラ：発射した弾の後ろを追いかけて映す
    class BulletCamera
    {
    public:
        // 弾の後方どれだけ離れるか
        float trailDistance = 1.5f;
        // 弾より少し上に上げる量
        float heightOffset = 0.25f;

        // 発射時：追従する弾をセット
        void OnFire(Bullet* bullet);
        // 弾を手放す（追従終了）
        void Detach();
        // 追従中の弾があるか
        bool HasBullet()   const { return m_hasBullet; }
        // 弾がまだ飛んでいるか
        bool BulletAlive() const;
        // 弾の位置からカメラ視点を計算
        void Update(Bullet* bullet);
        // ビュー行列を取得
        DirectX::SimpleMath::Matrix GetViewMatrix() const { return m_view; }
        // プロジェクション行列を取得
        DirectX::SimpleMath::Matrix GetProjectionMatrix(float aspectRatio) const;

    private:
        // 追従する弾（観測のみ・所有しない）
        Bullet* m_bullet = nullptr;
        // 弾を保持しているか
        bool m_hasBullet = false;
        // 計算したビュー行列
        DirectX::SimpleMath::Matrix m_view = DirectX::SimpleMath::Matrix::Identity;
    };
}