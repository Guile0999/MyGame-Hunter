#pragma once
#include <pch.h>
#include <Effects.h>
#include <SimpleMath.h>
#include <memory>
#include <vector>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <MyLib/Utilities/RaycastSystem.h>

namespace MyLib
{
    // 弾の状態
    enum class BulletState
    {
        Unuse,    // 未使用（プールで待機中）
        Use,      // 使用済み（命中または飛距離切れ）
        Flying    // 発射中（飛行してレイ判定を行う）
    };

    // 弾クラス：直進しながらレイ判定で命中を調べる
    class Bullet
    {
    public:
        // プール用のデフォルトコンストラクタ（空の弾を作る）
        Bullet();

        // コンストラクタ（発射情報を指定して作る）
        Bullet(
            const DirectX::SimpleMath::Vector3& origin,
            const DirectX::SimpleMath::Vector3& direction,
            float maxDistance,
            std::shared_ptr<DirectX::Model> model);

        // プールから再利用するときに撃ち直す（再初期化）
        void Reset(
            const DirectX::SimpleMath::Vector3& origin,
            const DirectX::SimpleMath::Vector3& direction,
            float maxDistance,
            std::shared_ptr<DirectX::Model> model);

        // 更新：移動＋レイ判定。今フレーム命中でtrueを返す
        bool Update(float deltaTime, RaycastSystem& raycaster, float speed = 75.0f);

        // 弾モデルの描画
        void Render(
            ID3D11DeviceContext* context,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj,
            DirectX::CommonStates& states);

        // トレイル描画（バレットカメラ中の軌跡）
        void RenderTrail(ID3D11DeviceContext* context,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);

        // 状態
        bool IsActive() const { return m_state == BulletState::Flying; }   // 飛行中か
        void SetActive(bool active) { m_state = active ? BulletState::Flying : BulletState::Use; }
        BulletState GetState() const { return m_state; }          // 状態の取得
        void SetState(BulletState state) { m_state = state; }     // 状態の設定

        // レイ情報を取得
        MyLib::Ray GetRay() const;

        // モデルの回転オフセット設定
        void SetRotationOffset(const DirectX::SimpleMath::Vector3& rot);

        // 命中情報の取得（読み取り専用）
        bool DidHit() const;                                          // 命中したか
        IRaycastTarget::HitPart GetHitPart() const;                   // 命中部位
        IRaycastTarget* GetHitTarget() const { return m_hitTarget; }  // 命中相手

        // ミス状態
        bool JustExpired() const;   // 飛距離切れで消滅したか

    private:
        
        // 位置・移動
        DirectX::SimpleMath::Vector3 m_position;    // 現在位置
        DirectX::SimpleMath::Vector3 m_direction;   // 進行方向
        float m_maxDistance;                        // 最大飛距離
        float m_traveledDistance;                   // 移動した距離
        BulletState m_state;                        // 弾の状態
     
        // 命中状態（保持される）
        bool m_didHit;                              // 命中したか
        IRaycastTarget::HitPart m_hitPart;          // 命中部位
        bool m_justExpired;                         // 消滅したか
        IRaycastTarget* m_hitTarget;                // 命中相手（所有しない）

        // 描画
        std::shared_ptr<DirectX::Model> m_model;
        DirectX::SimpleMath::Matrix m_world;        // ワールド行列
        DirectX::SimpleMath::Vector3 m_rotationOffset;   // モデルの回転オフセット

        // 弾道トレイル（軌跡）
        std::vector<DirectX::SimpleMath::Vector3> m_trailPoints;
        static constexpr size_t MAX_TRAIL_POINTS = 60;   // 軌跡の長さ
    };
}