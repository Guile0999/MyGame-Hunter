#include "pch.h"
#include "FirstPersonPlayer.h"
#include <algorithm>
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib {

    //=====================================================================
    // コンストラクタ
    //=====================================================================
    FirstPersonPlayer::FirstPersonPlayer(const PlayerSettings& settings)
        :
        m_settings(settings),                            // プレイヤー設定
        m_camera{},                                      // カメラ
        m_position(0.0f, 0.0f, 0.0f),                    // 位置
        m_velocity(0.0f, 0.0f, 0.0f),                    // 速度
        m_yaw(0.0f),                                     // 水平回転
        m_pitch(0.0f),                                   // 垂直回転
        m_mouseSkipFrames(3),                            // 最初の3フレームは無視
        m_isCrouching(false)                             // しゃがみ中か
    {
    }

    //=====================================================================
    // カメラ初期化
    //=====================================================================
    void FirstPersonPlayer::InitializeCamera(float fov, float nearZ, float farZ)
    {
        m_camera.Initialize(fov, nearZ, farZ);
    }

    //=====================================================================
    // カメラ情報取得
    //=====================================================================
    Vector3 FirstPersonPlayer::GetCameraPosition() const
    {
        return m_camera.GetPosition();
    }

    Vector3 FirstPersonPlayer::GetCameraForward() const
    {
        return m_camera.GetForward();
    }

    //=====================================================================
    // プレイヤー更新処理
    //  - マウス・キーボード入力
    //  - 移動・ジャンプ・しゃがみ・息止め
    //  - 重力・カメラ更新
    //=====================================================================
    void FirstPersonPlayer::Update(
        float deltaTime,
        const Keyboard::State& kb,
        const Keyboard::KeyboardStateTracker& kbTracker,
        const Mouse::State& mouse)
    {
        // マウス視点更新
        HandleMouseLook(mouse);
        // キーによる移動更新
        HandleMovement(deltaTime, kb);

        // しゃがみ切替（Shiftキー）
        if (kbTracker.IsKeyPressed(Keyboard::Keys::LeftShift))
            m_isCrouching = !m_isCrouching;

        // 重力適用
        m_velocity.y -= m_settings.gravity * deltaTime;

        // 速度を位置に反映
        m_position += m_velocity * deltaTime;

        // 地面衝突判定
        if (m_position.y < 0.0f)
        {
            m_position.y = 0.0f;
            m_velocity.y = 0.0f;
        }

        // カメラの回転・位置更新
        Matrix rotation = Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
        Vector3 cameraPosition = m_position + Vector3(0, eyeHeight, 0);
        m_camera.SetPosition(cameraPosition); // 目の高さに合わせる場合
        m_camera.SetRotation(m_pitch, m_yaw);
       
       
    }

    //=====================================================================
    // プレイヤー回転（Quaternion取得）
    //=====================================================================
    Quaternion FirstPersonPlayer::GetRotation() const
    {
        return Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
    }

    //=====================================================================
    // プレイヤー位置リセット
    //=====================================================================
    void FirstPersonPlayer::SetPosition(const Vector3& startPos)
    {
        m_position = startPos;
    }

    //=====================================================================
    // 移動処理
    //  - WASD入力に基づき前後左右移動
    //  - しゃがみ時は速度を低下
    //  - 入力なし時は摩擦で停止
    //=====================================================================
    void FirstPersonPlayer::HandleMovement(float deltaTime, const Keyboard::State& kb)
    {
        // 前方・右方向をyaw角から計算
        Vector3 forward(std::sin(m_yaw), 0, std::cos(m_yaw));
        Vector3 right(forward.z, 0, -forward.x);
        Vector3 moveDir = Vector3::Zero;

        // 前後左右移動
        if (kb.W) moveDir += forward;
        if (kb.S) moveDir -= forward;
        if (kb.A) moveDir += right;
        if (kb.D) moveDir -= right;

        if (moveDir.LengthSquared() > 0.1f)
        {
            moveDir.Normalize();
            float currentSpeed = m_isCrouching ? m_settings.crouchSpeed : m_settings.moveSpeed;
            // 水平速度を更新
            m_velocity.x = moveDir.x * currentSpeed;
            m_velocity.z = moveDir.z * currentSpeed;
        }
        else
        {
            // 入力なし時は摩擦で停止
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }
    }

    //=====================================================================
    // マウス視点処理
    //  - マウスの動きでYaw/Pitchを更新
    //  - Pitchは上下制限付き
    //=====================================================================
    void FirstPersonPlayer::HandleMouseLook(const Mouse::State& mouse)
    {
      
        if (m_mouseSkipFrames > 0)
        {
            m_mouseSkipFrames--;
            return;
        }
        m_yaw += mouse.x * m_settings.mouseSensitivity;
        m_pitch += mouse.y * m_settings.mouseSensitivity;
        // Pitch制限（ほぼ真上/真下まで）
        constexpr float pitchLimit = XMConvertToRadians(89.0f);
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

        // Yaw制限（左右）? 正面±90度＝合計180度まで
        constexpr float yawLimit = XMConvertToRadians(90.0f);
        m_yaw = std::clamp(m_yaw, -yawLimit, yawLimit);

    }

} // namespace MyLib