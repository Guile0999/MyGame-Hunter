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
    class Rabbit : public IRaycastTarget
    {
    public:
		// コンストラクタとデストラクタ
        Rabbit(const DirectX::SimpleMath::Vector3& pos = DirectX::SimpleMath::Vector3::Zero,
            float radius = 0.5f);
        ~Rabbit();

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

        // --- IRaycastTarget ---
        bool CheckRayHit(const Ray& ray, float& distance, HitPart& outPart) override;
        void OnRayHit(HitPart part) override;
        void ApplyHit(HitPart part);

        //ゲッター / セッター
        const DirectX::SimpleMath::Vector3& GetPosition()    const;
        float                               GetFacingAngle() const { return m_facingAngle; }
        bool                                IsAlive()        const { return m_alive; }
        void                                Kill() { m_alive = false; }
        void                                ShowCollisionBox(bool show) { m_showCollisionBox = show; }
        void                                SetWalking(bool walking) { m_isWalking = walking; }
        bool                                IsWalking()      const { return m_isWalking; }

		// コライダー中心のワールド座標を取得
        void SetMovementVariation(int startIndex, float speedScale);
        void SetWaypoints(const std::vector<DirectX::SimpleMath::Vector3>& points)
        {
            m_wayPoints = points;
        }
        bool IsDeadAnimDone() const { return m_deadAnimDone; }
        void rabbitSpook();
      
        DirectX::SimpleMath::Vector3 GetBodyCenter() const;
        DirectX::SimpleMath::Vector3 GetHeadCenter() const;

        const std::vector<DirectX::SimpleMath::Vector3>& GetWaypoints() const { return m_wayPoints; }

    private:
        DirectX::SimpleMath::Vector3 m_position;
        float                        m_radius;
        bool                         m_alive;
        bool                         m_isWalking;
        bool                         m_showCollisionBox;
        float                        m_facingAngle;

        //デバッグキューブ
        std::unique_ptr<DirectX::GeometricPrimitive> m_debugCube;

        //ライダーオフセット
        DirectX::SimpleMath::Vector3 m_bodyOffset;
        DirectX::SimpleMath::Vector3 m_headOffset;

        // OBBコライダー
        OBBCollider m_bodyCollider;
        OBBCollider m_headCollider;

        //サーマルオフセット
        DirectX::SimpleMath::Vector3 m_bodyThermalOffset = DirectX::SimpleMath::Vector3::Zero;
        DirectX::SimpleMath::Vector3 m_headThermalOffset = DirectX::SimpleMath::Vector3::Zero;

        // スプラインの移動
        std::vector<DirectX::SimpleMath::Vector3> m_wayPoints;
        float m_totalTime;
        float m_transformRatio;
        int   m_wayPointIndex;

       //モデル + アニメーション
        std::unique_ptr<DirectX::Model> m_model;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Idle;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Walk;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Run;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Dead;
        DX::AnimationSDKMESH* m_currentAnim;
        DirectX::ModelBone::TransformArray m_boneTransforms;

        // アニメーション状態
        enum class AnimState
        { Idle, 
          Walk, 
          Run, 
          Dead
        };
        // 死亡アニメーションの状態
        AnimState m_animState;
        float m_animTimer;
        int   m_walkLoopTarget;
        float m_idleDuration;
        bool  m_isDeadAnimating;
        float m_deadTimer;
        bool  m_deadAnimDone;
        float m_speedScale;

        bool  m_isSpooked;
        float m_spookTimer;
        static constexpr float SPOOK_DURATION = 4.0f;
        static constexpr float RUN_SPEED_SCALE = 2.0f;
    };
}