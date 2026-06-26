#include "pch.h"
#include "Bullet.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib {

    // デフォルトコンストラクタ（プール用の空の弾）
    Bullet::Bullet()
        :
        m_position(DirectX::SimpleMath::Vector3::Zero),      // 現在位置
        m_direction(DirectX::SimpleMath::Vector3::Forward),  // 進行方向
        m_maxDistance(0.0f),                                 // 最大飛距離
        m_traveledDistance(0.0f),                            // 移動した距離
        m_state(BulletState::Unuse),                         // 弾の状態（未使用）
        m_didHit(false),                                     // 命中したか
        m_hitPart(IRaycastTarget::HitPart::NONE),            // 命中部位
        m_justExpired(false),                                // 消滅したか
        m_hitTarget(nullptr),                                // 命中相手
        m_model(nullptr),                                    // 弾モデル
        m_world(DirectX::SimpleMath::Matrix::Identity),      // ワールド行列
        m_rotationOffset(0.0f, 0.0f, 0.0f)                   // 回転オフセット
    {
    }

    // コンストラクタ（発射情報を指定して各メンバを初期化子リストで初期化）
    Bullet::Bullet(
        const Vector3& origin,
        const Vector3& direction,
        float maxDistance,
        std::shared_ptr<Model> model)
        :
        m_position(origin),                                  // 現在位置
        m_direction(direction),                              // 進行方向
        m_maxDistance(maxDistance),                          // 最大飛距離
        m_traveledDistance(0.0f),                            // 移動した距離
        m_state(BulletState::Flying),                        // 弾の状態（発射中）
        m_didHit(false),                                     // 命中したか
        m_hitPart(IRaycastTarget::HitPart::NONE),            // 命中部位
        m_justExpired(false),                                // 消滅したか
        m_hitTarget(nullptr),                                // 命中相手
        m_model(model),                                      // 弾モデル（共有）
        m_world(DirectX::SimpleMath::Matrix::Identity),      // ワールド行列
        m_rotationOffset(0.0f, 0.0f, 0.0f)                   // 回転オフセット
    {
        // 方向を正規化（計算なので本体で行う）
        if (m_direction.LengthSquared() > 0.0001f)
            m_direction.Normalize();
    }

    // プールから再利用：発射情報をセットして飛行状態にする
    // ※既に構築済みのオブジェクトなので、初期化子リストではなく代入で行う
    void Bullet::Reset(
        const Vector3& origin,
        const Vector3& direction,
        float maxDistance,
        std::shared_ptr<Model> model)
    {
        m_position = origin;
        m_direction = direction;
        m_maxDistance = maxDistance;
        m_model = model;
        m_traveledDistance = 0.0f;
        m_state = BulletState::Flying;   // 発射状態にする

        // 命中状態をリセット
        m_didHit = false;
        m_hitPart = IRaycastTarget::HitPart::NONE;
        m_justExpired = false;
        m_hitTarget = nullptr;

        // 軌跡をクリア
        m_trailPoints.clear();

        // 方向を正規化
        if (m_direction.LengthSquared() > 0.0001f)
            m_direction.Normalize();
    }

    // 更新：移動とレイ判定。今フレームで命中したらtrueを返す
    bool Bullet::Update(float deltaTime, RaycastSystem& raycaster, float speed)
    {
        // 毎フレーム結果をリセット
        m_didHit = false;
        m_hitPart = IRaycastTarget::HitPart::NONE;
        m_justExpired = false;
        m_hitTarget = nullptr;

        if (!IsActive())
            return false;

        // 今フレームの移動量を計算
        Vector3 prevPos = m_position;
        Vector3 movement = m_direction * speed * deltaTime;
        float movementLength = movement.Length();

        if (movementLength <= 0.000001f)
            return false;

        Vector3 moveDir = movement / movementLength;

        // 移動区間をレイにして判定
        Ray travelRay(prevPos, moveDir);
        float hitDistance = 0.0f;
        IRaycastTarget* hitTarget = nullptr;
        IRaycastTarget::HitPart hitPart = IRaycastTarget::HitPart::NONE;

        // レイキャスト実行
        if (raycaster.ShootRay(travelRay, hitDistance, hitTarget, hitPart))
        {
            // 命中点が今フレームの移動範囲内なら命中確定
            if (hitDistance <= movementLength + 0.0001f)
            {
                m_position = prevPos + moveDir * hitDistance;
                m_traveledDistance += hitDistance;

                // 命中した相手に通知
                if (hitTarget)
                    hitTarget->OnRayHit(hitPart);

                m_state = BulletState::Use;   // 使用済みにする

                // 命中結果を保存
                m_didHit = true;
                m_hitPart = hitPart;
                m_hitTarget = hitTarget;
            }
        }

        // 命中していなければ通常移動
        if (IsActive())
        {
            m_position += movement;
            m_traveledDistance += movementLength;

            // 最大飛距離を超えたら消滅（＝ミス）
            if (m_traveledDistance >= m_maxDistance)
            {
                m_state = BulletState::Use;   // 使用済みにする
                m_justExpired = true;
            }
        }

        // 軌跡に現在位置を記録
        m_trailPoints.push_back(m_position);
        if (m_trailPoints.size() > MAX_TRAIL_POINTS)
            m_trailPoints.erase(m_trailPoints.begin());

        // ワールド行列を更新（弾を進行方向へ向ける）
        if (m_direction.LengthSquared() > 0.0001f)
        {
            Vector3 dir = m_direction;
            dir.Normalize();

            Matrix look = Matrix::CreateLookAt(Vector3::Zero, dir, Vector3::Up);
            look = look.Invert();

            Matrix align = Matrix::CreateRotationX(XMConvertToRadians(180.0f));

            Matrix rotOffset =
                Matrix::CreateRotationX(XMConvertToRadians(m_rotationOffset.y)) *
                Matrix::CreateRotationY(XMConvertToRadians(m_rotationOffset.x)) *
                Matrix::CreateRotationZ(XMConvertToRadians(m_rotationOffset.z));

            m_world = rotOffset * align * look * Matrix::CreateTranslation(m_position);
        }

        return m_didHit;
    }

    // 描画：弾モデルを表示
    void Bullet::Render(ID3D11DeviceContext* context,
        const Matrix& view, const Matrix& proj, CommonStates& states)
    {
        if (!m_model || !IsActive()) return;

        m_model->Draw(context, states, m_world, view, proj);
    }

    // トレイル描画（バレットカメラ中の軌跡をリボン状に描く）
    void Bullet::RenderTrail(ID3D11DeviceContext* context,
        const Matrix& view, const Matrix& proj)
    {
        if (m_trailPoints.size() < 2) return;

        // PrimitiveBatch と BasicEffect を初回だけ作る
        static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> batch;
        static std::unique_ptr<DirectX::BasicEffect> effect;
        static Microsoft::WRL::ComPtr<ID3D11InputLayout> layout;

        if (!batch)
        {
            Microsoft::WRL::ComPtr<ID3D11Device> device;
            context->GetDevice(device.GetAddressOf());

            batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
            effect = std::make_unique<DirectX::BasicEffect>(device.Get());
            effect->SetVertexColorEnabled(true);

            void const* shaderByteCode;
            size_t byteCodeLength;
            effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
            device->CreateInputLayout(
                DirectX::VertexPositionColor::InputElements,
                DirectX::VertexPositionColor::InputElementCount,
                shaderByteCode, byteCodeLength,
                layout.GetAddressOf());
        }

        effect->SetView(view);
        effect->SetProjection(proj);
        effect->SetWorld(Matrix::Identity);
        effect->Apply(context);
        context->IASetInputLayout(layout.Get());

        batch->Begin();

        // カメラ位置を取得（クアッドをカメラに向けるため）
        Vector3 camPos = view.Invert().Translation();
        float thickness = 0.15f;   // 軌跡の太さ

        // 点をつないでリボン状のクアッドを描く
        for (size_t i = 0; i + 1 < m_trailPoints.size(); ++i)
        {
            float a0 = (float)i / (float)m_trailPoints.size();
            float a1 = (float)(i + 1) / (float)m_trailPoints.size();

            DirectX::VertexPositionColor v0(m_trailPoints[i],
                Color(1.0f, 0.85f, 0.4f, a0));   // 黄色っぽい軌跡
            DirectX::VertexPositionColor v1(m_trailPoints[i + 1],
                Color(1.0f, 0.85f, 0.4f, a1));

            batch->DrawLine(v0, v1);
        }

        batch->End();
    }

    // 現在位置から前方へのレイを作って返す
    Ray Bullet::GetRay() const
    {
        MyLib::Ray ray;
        ray.origin = m_position;
        ray.direction = m_direction;
        return ray;
    }

    // モデルの回転オフセットを設定
    void Bullet::SetRotationOffset(const DirectX::SimpleMath::Vector3& rot)
    {
        m_rotationOffset = rot;
    }

    bool Bullet::DidHit() const
    {
        return m_didHit;
    }

    IRaycastTarget::HitPart Bullet::GetHitPart() const
    {
        return m_hitPart;
    }

    // 最大飛距離に達して消滅した（ミス）かどうか
    bool Bullet::JustExpired() const
    {
        return m_justExpired;
    }
}