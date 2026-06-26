#pragma once
#include "pch.h"
#include "TitleScene.h"
#include "Scene/TestScene.h"
#include "Scene/LoadScreen.h"
#include "SpriteBatch.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace MyLib;

void TitleScene::Initialize()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();
    // Create SpriteBatch
    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    // Load Title Logo texture
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(
            device,
            L"Resources/Textures/T3.png",
            resource.ReleaseAndGetAddressOf(),
            m_titleTexture.ReleaseAndGetAddressOf()
        )
    );
    // Load Play button texture
    Microsoft::WRL::ComPtr<ID3D11Resource> playRes;
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(
            device,
            L"Resources/Textures/Hover.png",
            playRes.ReleaseAndGetAddressOf(),
            m_playButtonTexture.ReleaseAndGetAddressOf()
        )
    );

    // Initialize fade and menu
    m_fadeAlpha = 0.5f;
    m_selectedIndex = 0;
    m_menuItems = {
                    L"Start Game ",
                    L"Exit" };

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();

    ClipCursor(nullptr);
    ShowCursor(TRUE);
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
}

void TitleScene::Update(float elapsedTime)
{

   
    //// Fade in the title logo
  
    m_time += elapsedTime;

    // Fade in title
    if (m_fadeAlpha < 1.0f)
        m_fadeAlpha = std::min(m_fadeAlpha + elapsedTime * 1.5f, 1.0f);

    // Menu fades in after title
    if (m_fadeAlpha >= 0.8f)
        m_menuFadeAlpha = std::min(m_menuFadeAlpha + elapsedTime * 2.0f, 1.0f);

    // Smooth selector slide
    float targetY = float(m_selectedIndex);
    m_selectorAnim += (targetY - m_selectorAnim) * elapsedTime * 12.0f;

    auto* kb = GetUserResources()->GetKeyboardStateTracker();
    if (!kb) return;

    if (kb->pressed.Down)
        m_selectedIndex = (m_selectedIndex + 1) % m_menuItems.size();
    else if (kb->pressed.Up)
        m_selectedIndex = (m_selectedIndex + int(m_menuItems.size()) - 1) % m_menuItems.size();

    if (kb->pressed.Enter)
    {
        switch (m_selectedIndex)
        {
        case 0: ChangeScene<TestScene, LoadScreen>(); break;
        case 1: PostQuitMessage(0); break;
        }
    }
}

void TitleScene::Render()
{
    RECT outputRect = GetUserResources()->GetDeviceResources()->GetOutputSize();
    float screenWidth = float(outputRect.right - outputRect.left);
    float screenHeight = float(outputRect.bottom - outputRect.top);

    // Button layout
    const float btnW = 168.0f;
    const float btnH = 88.0f;
    const float btnSpacing = 20.0f;
    const float totalH = btnH * m_menuItems.size() + btnSpacing * (m_menuItems.size() - 1);
    const float startX = (screenWidth - btnW) * 0.5f; // centered
    const float startY = (screenHeight - totalH) * 0.8f + 50.0f;

    m_spriteBatch->Begin();

    // „Ÿ„Ÿ Background title texture „Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ
    constexpr float TEX_W = 1536.0f, TEX_H = 1024.0f;
    float scale = std::max(screenWidth / TEX_W, screenHeight / TEX_H);
    Vector2 bgPos(
        (screenWidth - TEX_W * scale) * 0.5f,
        (screenHeight - TEX_H * scale) * 0.5f
    );
    m_spriteBatch->Draw(
        m_titleTexture.Get(), bgPos, nullptr,
        Color(1.f, 1.f, 1.f, m_fadeAlpha),
        0.0f, Vector2::Zero, Vector2(scale, scale)
    );

    // „Ÿ„Ÿ Draw each button „Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ
    for (size_t i = 0; i < m_menuItems.size(); ++i)
    {
        bool selected = (int(i) == m_selectedIndex);
        float btnY = startY + i * (btnH + btnSpacing);
        Vector2 btnPos(startX, btnY);

        // Pulse scale on selected button
        float btnScale = selected
            ? 1.0f + 0.04f * std::sin(m_time * 4.0f)
            : 1.0f;

        float alpha = m_menuFadeAlpha * (selected ? 1.0f : 0.6f);

        if (i == 0)
        {
          

            m_spriteBatch->Draw(
                m_playButtonTexture.Get(),
                btnPos,
                nullptr,
                Color(1.f, 1.f, 1.f, alpha),
                0.0f,
                Vector2::Zero,
                Vector2(btnScale, btnScale)
            );
        }
        else
        {
            // „Ÿ„Ÿ Other buttons ¨ dark box + text „Ÿ„Ÿ
            Color boxColor = selected
                ? Color(0.15f, 0.15f, 0.15f, alpha)  // dark grey box
                : Color(0.08f, 0.08f, 0.08f, alpha);

            // Draw dark background box
            m_spriteBatch->Draw(
                m_selectorTexture.Get(),
                btnPos,
                nullptr,
                boxColor,
                0.0f,
                Vector2::Zero,
                Vector2(btnW, btnH) * btnScale
            );

            // Draw border line on selected (red to match Hover.png style)
            if (selected)
            {
                // Top border
                m_spriteBatch->Draw(m_selectorTexture.Get(),
                    btnPos, nullptr, Color(0.8f, 0.1f, 0.3f, alpha),
                    0.0f, Vector2::Zero, Vector2(btnW, 2.0f));
                // Bottom border
                m_spriteBatch->Draw(m_selectorTexture.Get(),
                    Vector2(btnPos.x, btnPos.y + btnH - 2.0f), nullptr,
                    Color(0.8f, 0.1f, 0.3f, alpha),
                    0.0f, Vector2::Zero, Vector2(btnW, 2.0f));
                // Left border
                m_spriteBatch->Draw(m_selectorTexture.Get(),
                    btnPos, nullptr, Color(0.8f, 0.1f, 0.3f, alpha),
                    0.0f, Vector2::Zero, Vector2(2.0f, btnH));
                // Right border
                m_spriteBatch->Draw(m_selectorTexture.Get(),
                    Vector2(btnPos.x + btnW - 2.0f, btnPos.y), nullptr,
                    Color(0.8f, 0.1f, 0.3f, alpha),
                    0.0f, Vector2::Zero, Vector2(2.0f, btnH));
            }

            // Draw text centered in button
            XMVECTOR strSize = m_menuFont->MeasureString(m_menuItems[i].c_str());
            float textW = XMVectorGetX(strSize) * btnScale;
            float textH = XMVectorGetY(strSize) * btnScale;

            Vector2 textPos(
                btnPos.x + (btnW - textW) * 0.5f,
                btnPos.y + (btnH - textH) * 0.5f
            );

            Color textColor = selected
                ? Color(1.0f, 1.0f, 1.0f, alpha)
                : Color(0.6f, 0.6f, 0.6f, alpha);

            m_menuFont->DrawString(
                m_spriteBatch.get(),
                m_menuItems[i].c_str(),
                textPos, textColor,
                0.0f, Vector2::Zero, btnScale
            );
        }
    }

    m_spriteBatch->End();

    // „Ÿ„Ÿ Hint text „Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ„Ÿ
    auto* debugFont = GetUserResources()->GetDebugFont();
    float hintAlpha = m_menuFadeAlpha * (0.4f + 0.2f * std::sin(m_time * 2.0f));
  
    debugFont->Render(GetUserResources()->GetCommonStates());
  
   
}

void TitleScene::Finalize()
{
}

void TitleScene::CreateDeviceDependentResources()
{
    auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();

    // 1x1 white texture for other buttons background
    uint32_t whitePixel = 0xFFFFFFFF;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &whitePixel;
    data.SysMemPitch = sizeof(uint32_t);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    DX::ThrowIfFailed(device->CreateTexture2D(&desc, &data, tex.GetAddressOf()));
    DX::ThrowIfFailed(device->CreateShaderResourceView(
        tex.Get(), nullptr, m_selectorTexture.ReleaseAndGetAddressOf()));

    // Load menu font
    m_menuFont = std::make_unique<DirectX::SpriteFont>(
        device, L"Resources/Font/SegoeUI_18.spritefont"
    );
}

void TitleScene::CreateWindowSizeDependentResources()
{
}

void TitleScene::OnDeviceLost()
{
}
