#pragma once
#include "pch.h"
#include "Objects/FpsPlayerSetting.h"
#include "Objects/FpsCamera.h"

namespace MyLib {
    class FirstPersonPlayer
    {
    public:
        FirstPersonPlayer(const PlayerSettings& settings);                  // コンストラクタ
        void InitializeCamera(float fov, float nearZ, float farZ);          // カメラ初期化

        // カメラ情報取得
        const DirectX::SimpleMath::Matrix& GetViewMatrix() const { return m_camera.GetViewMatrix(); }
        DirectX::SimpleMath::Matrix GetProjectionMatrix(float aspectRatio) const { return m_camera.GetProjectionMatrix(aspectRatio); }
        const FpsCamera& GetCamera() const { return m_camera; }
        DirectX::SimpleMath::Vector3 GetCameraPosition() const;
        DirectX::SimpleMath::Vector3 GetCameraForward() const;

        // 更新処理
        void Update(float deltaTime, const DirectX::Keyboard::State& kb, const DirectX::Keyboard::KeyboardStateTracker& kbTracker, const DirectX::Mouse::State& mouse);

        // プレイヤーの位置・回転取得
        const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }
        DirectX::SimpleMath::Quaternion GetRotation() const;
        float GetYaw() const { return m_yaw; }
        float GetPitch() const { return m_pitch; }

        // プレイヤー位置リセット
        void SetPosition(const DirectX::SimpleMath::Vector3& startPos);
        // 視点方向リセット (yaw/pitch in radians)
        void SetLook(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; }

    private:
        void HandleMovement(float deltaTime, const DirectX::Keyboard::State& kb);   // 移動処理
        void HandleMouseLook(const DirectX::Mouse::State& mouse);                   // マウス視点処理

    private:
        PlayerSettings m_settings;                      // プレイヤー設定
        FpsCamera m_camera;                             // カメラ
        DirectX::SimpleMath::Vector3 m_position;        // 位置
        DirectX::SimpleMath::Vector3 m_velocity;        // 速度
        float m_yaw;                                    // 水平回転（左右）
        float m_pitch;                                  // 垂直回転（上下）
        int   m_mouseSkipFrames;                        // 最初の数フレームのマウス入力を無視
        const float eyeHeight = 1.7f;                   // 目の高さ（定数）
        bool m_isCrouching;                             // しゃがみ中か
    };
}