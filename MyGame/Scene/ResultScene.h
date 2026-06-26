#pragma once
#include <pch.h>
#include <Scene/SceneManager.h>
#include <UserResources.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include "Scene/TestScene.h"

namespace MyLib
{
    
    // ミッション結果（成功/失敗）を表示するシーン
    class ResultScene : public Scene<UserResources>
    {
    public:
        ResultScene() = default;

        // リソースの読み込み
        void Initialize() override;
        // 更新/アニメーション処理
        void Update(float elapsedTime) override;
        // 結果の描画
        void Render() override;
        // 後始末
        void Finalize() override;

    private:

        enum class Phase { ScorePanel, Result };
        Phase m_phase = Phase::ScorePanel;

        // 現在のミッション結果
        UserResources::MissionResult m_result;
        // 2D描画用
        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
        // 成功画像
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_successTexture;
        // 失敗画像
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_failedTexture;
		// フォント（必要に応じて使用）
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_panelTexture;

		// フォント（必要に応じて使用）
        std::unique_ptr<DirectX::SpriteFont> m_font;
        int m_finalScore = 0;

        DirectX::Mouse::ButtonStateTracker m_mouseTracker;
        bool m_advanced = false;
        bool m_buttonReleased = false;
    };
}