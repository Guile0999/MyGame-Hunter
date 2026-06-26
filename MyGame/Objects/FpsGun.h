#pragma once
#include <pch.h>
#include <Objects/FpsGunSetting.h>
#include <Objects/Bullet.h>
#include <SimpleMath.h>
#include <memory>

namespace MyLib {
    class FirstPersonPlayer;
    class RaycastSystem;

    class FpsGun
    {
    public:
        FpsGun();

        // 初期化
        void Initialize(const FpsGunSetting& setting);

        // 更新 

        void Update(const FirstPersonPlayer& player);

        // 発射
        Bullet Shoot(const FirstPersonPlayer& player, std::shared_ptr<DirectX::Model> bulletModel, RaycastSystem& raycaster);

        //ワールド行列取得
        const DirectX::SimpleMath::Matrix& GetWorldMatrix() const { return m_world; }
        //前方方向取得
        DirectX::SimpleMath::Vector3 GetForward() const;

        // 銃口のワールド座標取得
        DirectX::SimpleMath::Vector3 GetMuzzlePositionInWorld() const;

       /* void SetBoltOffset(const DirectX::SimpleMath::Vector3& offset)
        {
            m_boltOffset = offset;
        }*/
    private:
        // 銃の位置
        DirectX::SimpleMath::Vector3 m_position;
        // 銃の回転
        DirectX::SimpleMath::Vector3 m_rotation;
        // 銃の設定（弾速、リコイル等）
        FpsGunSetting m_setting;
        // ワールド行列
        DirectX::SimpleMath::Matrix m_world;
        // 射程距離
        float m_fireRange = 50.0f;

		//// ボルトの位置オフセット（アニメーション用）
  //      DirectX::SimpleMath::Vector3 m_boltOffset = { 0.0f, 0.0f, 0.0f };
    };
}