#include "pch.h"
#include "Objects/FpsCamera.h"
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX::SimpleMath;

namespace MyLib
{
   
    // コンストラクタ：カメラの初期位置・回転・投影パラメータを設定
    FpsCamera::FpsCamera()
        : m_position(0.0f, 1.8f, -5.0f),
        m_pitch(0.0f),
        m_yaw(0.0f),
        m_fovRadians(DirectX::XMConvertToRadians(45.0f)),
        m_nearZ(0.1f),
        m_farZ(1000.0f)
    {
        UpdateVectors(); // 前・右・上ベクトル計算
    }

    
    // カメラ投影パラメータ初期化（FOV, near/far plane）
    void FpsCamera::Initialize(float fovDegrees, float nearZ, float farZ)
    {
        m_fovRadians = DirectX::XMConvertToRadians(fovDegrees);
        m_nearZ = nearZ;
        m_farZ = farZ;
        m_pitch = 0.0f;
        m_yaw = 0.0f;
        UpdateVectors();
    }

   
    // カメラ回転更新（マウス入力やDelta値でYaw/Pitchを変更）
    void FpsCamera::Update(float dt, float deltaX, float deltaY)
    {
        const float sensitivity = 0.002f;

        m_yaw += deltaX * sensitivity;   // 横回転
        m_pitch += deltaY * sensitivity; // 縦回転

        // ピッチ制限（上下の180度反転を防ぐ）
        const float pitchLimit = 90.0f * DirectX::XM_PI / 180.0f;
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

        UpdateVectors(); // ベクトル再計算
    }

    
    // カメラ移動
    void FpsCamera::MoveForward(float amount) { m_position += m_forward * amount; }
    void FpsCamera::Strafe(float amount) { m_position += m_right * amount; }
    void FpsCamera::MoveUp(float amount) { m_position += m_up * amount; }

    
    // 位置・回転設定
    void FpsCamera::SetPosition(const Vector3& pos) { m_position = pos; }

    void FpsCamera::SetRotationMatrix(const Matrix& rot)
    {
        // 行列から前方向ベクトルを抽出
        Vector3 forward = Vector3::Transform(Vector3::UnitZ, rot);

        // 前方向からYaw/Pitchを計算
        m_yaw = std::atan2(forward.x, forward.z);
        m_pitch = std::asin(forward.y);
        UpdateVectors();
    }

    void FpsCamera::SetRotation(float pitch, float yaw)
    {
        float pitchLimit = DirectX::XM_PIDIV2 - 0.01f; // 上下制限
        m_pitch = std::clamp(pitch, -pitchLimit, pitchLimit);
        m_yaw = yaw;
        UpdateVectors();
    }
    // カメラ情報取得
    Vector3 FpsCamera::GetPosition() const { return m_position; }
    Vector3 FpsCamera::GetForward() const { return m_forward; }
    Vector3 FpsCamera::GetRight() const { return m_right; }
    Vector3 FpsCamera::GetUp() const { return m_up; }

    // ビュー行列取得
    Matrix FpsCamera::GetViewMatrix() const
    {
        return Matrix::CreateLookAt(m_position, m_position + m_forward, m_up);
    }

    // プロジェクション行列取得
    Matrix FpsCamera::GetProjectionMatrix(float aspectRatio) const
    {
        return Matrix::CreatePerspectiveFieldOfView(m_fovRadians, aspectRatio, m_nearZ, m_farZ);
    }
    
    // 前・右・上ベクトルをYaw/Pitchから再計
    void FpsCamera::UpdateVectors()
    {
        Matrix rotation = Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
        m_forward = Vector3::Transform(Vector3::UnitZ, rotation);
        m_right = Vector3::Transform(Vector3::UnitX, rotation);
        m_up = Vector3::Transform(Vector3::UnitY, rotation);
    }
}