#pragma once
#include <pch.h>
#include <SimpleMath.h>
#include <memory>
#include <GeometricPrimitive.h>
#include <Model.h>
#include <Effects.h>
#include <MyLib/Utilities/IRaycastTarget.h>
#include <MyLib/Utilities/Ray.h>
#include <MyLib/Utilities/Animation.h>
#include <Collision/Collision.h>

namespace MyLib
{
    // 鹿クラス：経路移動・アニメーション・当たり判定を持つ標的
    class Deer : public IRaycastTarget
    {
    public:
        Deer(const DirectX::SimpleMath::Vector3& pos = DirectX::SimpleMath::Vector3::Zero,
            float radius = 1.0f);
        ~Deer();

        void Initialize(ID3D11DeviceContext* context);   // 初期化
        void Update(float deltaTime);                    // 更新（移動・コライダー同期）
        void Render(const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);    // デバッグ用ボックス描画

        //モデル＋アニメーション（鹿が所有）
        void LoadModel(ID3D11Device* device);            // モデル・アニメ読み込み
        void UpdateAnimation(float deltaTime);           // アニメーション更新
        void DrawModel(ID3D11DeviceContext* context, DirectX::CommonStates& states,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);    // スキンメッシュ描画

        //IRaycastTarget インターフェース 
        bool CheckRayHit(const Ray& ray, float& distance, HitPart& outPart) override;  // レイ判定

        // 命中時の反応
        void ApplyHit(HitPart part);
        void OnRayHit(HitPart part) override;

        // ミス時に呼ぶ — 鹿が逃げる（走りアニメ＋加速）
        void Spook();

        //ゲッター / セッター
        const DirectX::SimpleMath::Vector3& GetPosition() const;
        void SetPosition(const DirectX::SimpleMath::Vector3& pos);
        void SetWaypoints(const std::vector<DirectX::SimpleMath::Vector3>& points)
        {
            m_wayPoints = points;
        }
        float GetRadius() const;
        bool IsAlive() const;
        void Kill();
        float GetFacingAngle() const { return m_facingAngle; }
        void ShowCollisionBox(bool show) { m_showCollisionBox = show; }   // 判定ボックス表示切替
        void SetWalking(bool walking) { m_isWalking = walking; }
        bool IsWalking() const { return m_isWalking; }
        void SetMovementVariation(int startIndex, float speedScale);      // 移動のばらつき設定

        // 死亡アニメ完了判定（キルカウント用）
        bool IsDeadAnimDone() const { return m_deadAnimDone; }

        // サーマル／命中位置
        DirectX::SimpleMath::Vector3 GetBodyCenter() const;
        DirectX::SimpleMath::Vector3 GetHeadCenter() const;

    private:
        // 位置・状態
        DirectX::SimpleMath::Vector3 m_position;
        float m_radius;
        bool m_alive;

        // デバッグ用キューブ
        std::unique_ptr<DirectX::GeometricPrimitive> m_debugCube;
        std::unique_ptr<DirectX::GeometricPrimitive> m_bodyBox;
        std::unique_ptr<DirectX::GeometricPrimitive> m_headBox;

        // 待機時の基準位置
        DirectX::SimpleMath::Vector3 m_basePosition;

        // コライダーのオフセット
        DirectX::SimpleMath::Vector3 m_bodyOffset;
        DirectX::SimpleMath::Vector3 m_headOffset;

        // OBBコライダー（体・頭）
        OBBCollider m_bodyCollider;
        OBBCollider m_headCollider;
        DirectX::SimpleMath::Vector3 m_bodyThermalOffset;
        DirectX::SimpleMath::Vector3 m_headThermalOffset;

        bool m_showCollisionBox;
        bool m_isWalking;

        // スプライン移動（Catmull-Rom）
        std::vector<DirectX::SimpleMath::Vector3> m_wayPoints;
        float m_totalTime;
        float m_transformRatio;
        int   m_wayPointIndex;
        float m_facingAngle;

        // 頭ボーン追従の当たり判定用
        static constexpr int HEAD_BONE_INDEX = 26;   // 「Head」ボーンのインデックス
        DirectX::SimpleMath::Vector3 m_animatedHeadPos;   // アニメから取得した頭の位置
        bool m_hasAnimatedHead;

        //モデル＋アニメーション（鹿ごとに所有）
        std::unique_ptr<DirectX::Model> m_model;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Idle;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Walk;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Run;
        std::unique_ptr<DX::AnimationSDKMESH> m_anim_Dead;
        DX::AnimationSDKMESH* m_currentAnim;
        DirectX::ModelBone::TransformArray m_boneTransforms;

        // 逃走状態
        bool  m_isSpooked;
        float m_spookTimer;
        static constexpr float SPOOK_DURATION = 4.0f;    // 4秒間逃げる
        static constexpr float RUN_SPEED_SCALE = 7.5f;   // 逃走中の速度倍率

        // アニメーション状態
        enum class AnimState { Idle, Walk, Run, Dead };
        AnimState m_animState;
        float m_animTimer;
        int   m_walkLoopTarget;
        float m_idleDuration;
        bool  m_isDeadAnimating;
        float m_deadTimer;
        bool  m_deadAnimDone;
        float m_speedScale;
    };
}