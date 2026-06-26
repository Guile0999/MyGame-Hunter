// FpsCamera.h
#pragma once
#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "MyLib/Utilities/Singleton.h"

namespace MyLib {

    // 一人称視点カメラクラス（シングルトン）
    class FpsCamera : public Singleton<FpsCamera>
    {
    public:
        FpsCamera();
        ~FpsCamera() = default;

        //=====================================================================
        // カメラ初期化
        // fovDegrees: 視野角（デフォルト45°）
        // nearZ: ニアクリップ平面
        // farZ: ファークリップ平面
        //=====================================================================
        void Initialize(float fovDegrees = 45.0f, float nearZ = 0.1f, float farZ = 1000.0f);

        //=====================================================================
        // カメラ回転更新（マウスDeltaでYaw/Pitchを更新）
        // dt: 経過時間
        // deltaX/Y: マウス移動量
        //=====================================================================
        void Update(float dt, float deltaX, float deltaY);

        //=====================================================================
        // カメラ移動
        //=====================================================================
        void MoveForward(float amount); // 前後移動
        void Strafe(float amount);      // 左右移動
        void MoveUp(float amount);      // 上下移動

        //=====================================================================
        // ビュー・プロジェクション行列取得
        //=====================================================================
        DirectX::SimpleMath::Matrix GetViewMatrix() const;
        DirectX::SimpleMath::Matrix GetProjectionMatrix(float aspectRatio) const;

        //=====================================================================
        // 位置・回転設定
        //=====================================================================
        void SetPosition(const DirectX::SimpleMath::Vector3& pos);
        void SetRotationMatrix(const DirectX::SimpleMath::Matrix& rot);
        void SetRotation(float pitch, float yaw);

        //=====================================================================
        // 位置・方向ベクトル取得
        //=====================================================================
        DirectX::SimpleMath::Vector3 GetPosition() const;
        DirectX::SimpleMath::Vector3 GetForward() const;
        DirectX::SimpleMath::Vector3 GetRight() const;
        DirectX::SimpleMath::Vector3 GetUp() const;
        DirectX::SimpleMath::Matrix GetRotationMatrix() const {
            return DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
        };

    private:
        // 前方・右・上方向ベクトルを再計算
        void UpdateVectors();

        //=====================================================================
        // メンバ変数
        //=====================================================================
        DirectX::SimpleMath::Vector3 m_position; // ワールド空間でのカメラ位置
        DirectX::SimpleMath::Vector3 m_forward;  // 前方向ベクトル
        DirectX::SimpleMath::Vector3 m_right;    // 右方向ベクトル
        DirectX::SimpleMath::Vector3 m_up;       // 上方向ベクトル

        float m_pitch;       // ピッチ角（縦回転ラジアン）
        float m_yaw;         // ヨー角（横回転ラジアン）

        float m_fovRadians;  // 視野角（ラジアン）
        float m_nearZ;       // ニアクリップ平面
        float m_farZ;        // ファークリップ平面
    };
}