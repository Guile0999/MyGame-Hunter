#include <pch.h>
#include "Scene/ResultScene.h"
#include "Scene/TitleScene.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    void ResultScene::Initialize()
    {
        // Get mission result from UserResources
        m_result = GetUserResources()->missionResult;
        m_finalScore = GetUserResources()->finalScore;
        m_phase = Phase::Result;

        auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
        auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();
        // SpriteBatchを作成（2D描画用）
        m_spriteBatch = std::make_unique<SpriteBatch>(context);

        // 成功テクスチャの読み込み
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(
                device,
                L"Resources/Textures/ResultScene.png",
                nullptr,
                m_successTexture.ReleaseAndGetAddressOf()
            )
        );

        //失敗テクスチャの読み込み 
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(
                device,
                L"Resources/Textures/MissionFailed.png",
                nullptr,
                m_failedTexture.ReleaseAndGetAddressOf()
            )
        );

        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/Big.png",
            nullptr, m_panelTexture.ReleaseAndGetAddressOf()));

        // フォント読み込み (ADD THIS)
        m_font = std::make_unique<SpriteFont>(device,
            L"Resources/Font/SegoeUI_18.spritefont");
    }

    void ResultScene::Update(float)
    {
        m_mouseTracker.Update(Mouse::Get().GetState());
        auto mouse = Mouse::Get().GetState();

        // wait for button release before accepting a click (avoid carryover from previous scene)
        if (!m_buttonReleased)
        {
            if (!mouse.leftButton)
                m_buttonReleased = true;
            return;
        }

        if (!m_advanced &&
            m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
        {
            m_advanced = true;
            if (m_result == UserResources::MissionResult::Success)
            {
                GetUserResources()->currentStage++;
                if (GetUserResources()->currentStage > 3)
                    GetUserResources()->currentStage = 1;
                if (GetUserResources()->currentStage < 1)
                    GetUserResources()->currentStage = 1;
            }
            GetUserResources()->missionResult = UserResources::MissionResult::None;
            ChangeScene<TestScene>();
        }
    }

    void ResultScene::Render()
    {
       
        auto deviceResources = GetUserResources()->GetDeviceResources();
        RECT rect = deviceResources->GetOutputSize();
        float screenWidth = float(rect.right);
        float screenHeight = float(rect.bottom);

        if (m_phase == Phase::ScorePanel)
        {
			// --- Phase 1:スコア　パネル ---
            float panelW = 606.0f, panelH = 454.0f;
            float scale = 1.0f;
            Vector2 panelPos(
                (screenWidth - panelW * scale) * 0.5f,
                (screenHeight - panelH * scale) * 0.5f);

            m_spriteBatch->Begin();
            m_spriteBatch->Draw(m_panelTexture.Get(), panelPos, nullptr,
                Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));
            m_spriteBatch->End();

            if (m_font)
            {
                m_spriteBatch->Begin();

                // Title: "OBJECTIVE CLEAR" or "MISSION FAILED"
                const wchar_t* title =
                    (m_result == UserResources::MissionResult::Success)
                    ? L"OBJECTIVE CLEAR" : L"MISSION FAILED";
                XMVECTOR tsz = m_font->MeasureString(title);
                float tw = XMVectorGetX(tsz) * 1.5f;
                m_font->DrawString(m_spriteBatch.get(), title,
                    Vector2(screenWidth * 0.5f - tw * 0.5f, screenHeight * 0.38f),
                    Colors::Gold, 0.0f, Vector2::Zero, 1.5f);

                //現金
                wchar_t cashBuf[64];
                swprintf_s(cashBuf, L"$ %d", m_finalScore);
                XMVECTOR csz = m_font->MeasureString(cashBuf);
                float cw = XMVectorGetX(csz) * 2.0f;
                m_font->DrawString(m_spriteBatch.get(), cashBuf,
                    Vector2(screenWidth * 0.5f - cw * 0.5f, screenHeight * 0.50f),
                    Colors::White, 0.0f, Vector2::Zero, 2.0f);

                // ヒント
                const wchar_t* hint = L"Left Click to continue";
                XMVECTOR hsz = m_font->MeasureString(hint);
                float hw = XMVectorGetX(hsz) * 0.8f;
                m_font->DrawString(m_spriteBatch.get(), hint,
                    Vector2(screenWidth * 0.5f - hw * 0.5f, screenHeight * 0.62f),
                    Colors::LightGray, 0.0f, Vector2::Zero, 0.8f);

                m_spriteBatch->End();
            }
        }
        else
        {
			// --- Phase 2:　結果画像 ---
            constexpr float TEX_WIDTH = 1536.0f, TEX_HEIGHT = 1024.0f;
            float scale = std::max(screenWidth / TEX_WIDTH, screenHeight / TEX_HEIGHT);
            Vector2 position(
                (screenWidth - TEX_WIDTH * scale) * 0.5f,
                (screenHeight - TEX_HEIGHT * scale) * 0.5f);

            ID3D11ShaderResourceView* texture =
                (m_result == UserResources::MissionResult::Success)
                ? m_successTexture.Get() : m_failedTexture.Get();

            m_spriteBatch->Begin();
            m_spriteBatch->Draw(texture, position, nullptr,
                Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));
            m_spriteBatch->End();
        }
    }

    void ResultScene::Finalize()
    {
    }
}