#pragma once
#include <pch.h>
#include <SimpleMath.h>
#include <memory>
#include <vector>
#include <GeometricPrimitive.h>
#include <Model.h>
#include <Effects.h>
#include <MyLib/Utilities/IRaycastTarget.h>
#include <MyLib/Utilities/Ray.h>
#include <MyLib/Utilities/Animation.h>
#include <Collision/Collision.h>

namespace MyLib
{
    class Boss : public IRaycastTarget
    {
    public:
		// コンストラクタとデストラクタ
        Boss(const DirectX::SimpleMath::Vector3& pos = DirectX::SimpleMath::Vector3::Zero,
            float radius = 1.5f);
        ~Boss();
		// 初期化、更新、描画
        void Initialize(ID3D11DeviceContext* context);
        void Update(float deltaTime);
        void Render(const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);

		// デバイスリソースの管理　　　　　　　　　　　
        void LoadModel(ID3D11Device* device);
        void UpdateAnimation(float deltaTime);
        void DrawModel(ID3D11DeviceContext* context, DirectX::CommonStates& states,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);
        bool IsDeadAnimDone() const { return m_deadAnimDone; }

        // --- IRaycastTarget ---
        bool CheckRayHit(const Ray& ray, float& distance, HitPart& outPart) override;
        void OnRayHit(HitPart part) override;
        void ApplyHit(HitPart part);

       //ゲッター
        const DirectX::SimpleMath::Vector3& GetPosition()    const;
        float                               GetFacingAngle() const { return m_facingAngle; }
        bool                                IsAlive()        const { return m_alive; }
        void                                Kill() { m_alive = false; }
        void                                ShowCollisionBox(bool show) { m_showCollisionBox = show; }
        void                                SetWalking(bool walking) { m_isWalking = walking; }
        bool                                IsWalking()      const { return m_isWalking; }

		// コライダー中心のワールド座標を取得
        DirectX::SimpleMath::Vector3 GetHeadCenter() const;
        DirectX::SimpleMath::Vector3 GetLungCenter() const;

        // チェイスシステム
        void  SetPlayerPosition(const DirectX::SimpleMath::Vector3& pos) { m_playerPosition = pos; }
        bool  IsCharging()     const { return m_isCharging; }
        bool  IsAttacking()    const { return m_isAttacking; }
        bool  HasReachedPlayer() const;
        void  StartCharging();
        bool HasHitPlayer() const { return m_hasHitPlayer; }
        void ResetHitPlayer() { m_hasHitPlayer = false; }
        void  GenerateChargeWaypoints();
        bool  WasKilledByLethalShot() const { return m_killedByLethalShot; }
        void  ResetKilledFlag() { m_killedByLethalShot = false; }
        DirectX::SimpleMath::Vector3 GetBodyCenter() const;
    private:
        DirectX::SimpleMath::Vector3 m_position;
        float                        m_radius;
        bool                         m_alive;
        bool                         m_isWalking;
        bool                         m_showCollisionBox;
        float                        m_facingAngle;

        /// デバッグキューブ
        std::unique_ptr<DirectX::GeometricPrimitive> m_debugCube;

		// コライダーオフセット
        DirectX::SimpleMath::Vector3 m_bodyOffset;
        DirectX::SimpleMath::Vector3 m_headOffset;

		// コライダー
        OBBCollider m_bodyCollider;
        OBBCollider m_headCollider;

		// 頭のアニメーション位置
        DirectX::SimpleMath::Vector3 m_headThermalOffset = DirectX::SimpleMath::Vector3::Zero;

		// 死亡フラグ（致命的な一撃で倒されたかどうか）
        bool m_killedByLethalShot;

		// 肺のコライダーとオフセット
        OBBCollider m_lungCollider;
        DirectX::SimpleMath::Vector3 m_lungOffset;
        DirectX::SimpleMath::Vector3 m_lungThermalOffset = DirectX::SimpleMath::Vector3::Zero;

		// チェイスと攻撃の状態
        int   m_hitCount;
        bool  m_isCharging;
        bool  m_isAttacking;
        bool  m_lungHitOnce;
        float m_chargeSpeed;
        float m_attackRange;
        float m_attackTimer;
        float m_attackDuration;
        bool  m_hasAttacked;
        DirectX::SimpleMath::Vector3 m_playerPosition = DirectX::SimpleMath::Vector3::Zero;

		// チャージ移動
        std::vector<DirectX::SimpleMath::Vector3> m_chargeWayPoints;
        int   m_chargeWayPointIndex;
        float m_chargeTotalTime;
        float m_chargeTransformRatio;
        bool m_hasHitPlayer;

		// チャージ後の移動（プレイヤーを追いかける）
        std::vector<DirectX::SimpleMath::Vector3> m_wayPoints;
        float m_totalTime;
        float m_transformRatio;
        int   m_wayPointIndex;

		// モデルとアニメーション
        std::unique_ptr<DirectX::Model> m_model;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Idle;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Walk;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Run;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Attack;
        DX::AnimationSDKMESH* m_currentAnim;
        DirectX::ModelBone::TransformArray m_boneTransforms;

		// アニメーション状態
        enum class AnimState 
        { 
            Idle,
            Walk,
            Run,
            Attack
        };
		// 死亡アニメーションの状態
        AnimState m_animState;
        float m_animTimer;
        int   m_walkLoopTarget;
        float m_idleDuration;
        float m_deadTimer;
        bool  m_deadAnimDone;
    };
}