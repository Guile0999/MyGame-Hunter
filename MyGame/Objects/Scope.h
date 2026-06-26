#pragma once
#include <pch.h>
#include <SpriteBatch.h>
#include <SimpleMath.h>
#include <WICTextureLoader.h>
#include <wrl/client.h> 
#include <UserResources.h>

namespace MyLib
{
    enum class ScopeStates
    {
        Inactive,
        Zoom1,
        Zoom2
    };

    class Scope
    {
    public:
        Scope();
        void Initialize(UserResources* resources);                          // 初期化
        void CycleState();                                                  // スコープ状態切り替え
        void Update(float elapsedTime, bool holdingBreath);                 // 更新
        void Render(const DirectX::SimpleMath::Vector2& screenSize);        // 描画
        float GetZoomFactor() const;                                        // ズーム倍率取得

        // 息の残量（0=満タン、1=使い切り）
        float GetBreathFraction() const { return m_breathTime / BREATH_MAX; }
        // 状態チェック
        bool IsActive() const { return m_state != ScopeStates::Inactive; }
        bool IsHoldingBreath() const { return m_holdingBreath; }

    private:
        // 現在の状態
        ScopeStates m_state;
        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
        // スコープ画像
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scopeTexture;
        // 息ゲージ用の白テクスチャ
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteTexture;

        // ズームレベル
        float m_zoom1;
        float m_zoom2;

        // 息止め（ブレスホールド）
        float m_breathTime;                              // 息を使った時間
        static constexpr float BREATH_MAX = 3.0f;        // 息を止められる最大秒数
        static constexpr float BREATH_RECOVERY = 1.5f;   // 息の回復速度

        // 揺れパラメータ
        float m_shakeAmount;
        DirectX::SimpleMath::Vector2 m_currentShake;
        float m_shakeTime;
        bool m_holdingBreath;

        // ユーザーリソースへの参照
        UserResources* m_resources;
    };
}