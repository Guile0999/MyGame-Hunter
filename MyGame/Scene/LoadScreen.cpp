#include "pch.h"
#include "LoadScreen.h"
#include "Scene/TestScene.h"

#include <WICTextureLoader.h>
#include <SimpleMath.h>
#include "SpriteBatch.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
/*--------------------------------------------------
コンストラクタ
--------------------------------------------------*/
LoadScreen::LoadScreen()
{
}
/*--------------------------------------------------
デストラクタ
--------------------------------------------------*/
LoadScreen::~LoadScreen()
{
}
/*--------------------------------------------------
初期化
--------------------------------------------------*/

void LoadScreen::Initialize()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	////Create SpriteBatch
	//m_spriteBatch = std::make_unique<SpriteBatch>(context);
	//Load Title Logo texture
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	DX::ThrowIfFailed(
		CreateWICTextureFromFile(
			device,
			L"Resources/Textures/LoadTexture.png",
			resource.ReleaseAndGetAddressOf(),
			m_loadingTexture.ReleaseAndGetAddressOf()
		)
	);
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
/*--------------------------------------------------
更新
戻り値	:次のシーン番号
--------------------------------------------------*/
void LoadScreen::Update(float elapsedTime)
{
	
    m_elapsed += elapsedTime;
    float target = m_loadingDone ? 1.0f : 0.9f;
    m_progress += (target - m_progress) * elapsedTime * 4.0f;
    m_progress = std::min(m_progress, target);
   
}
/*--------------------------------------------------
描画
--------------------------------------------------*/
void LoadScreen::Render()
{
    if (!m_spriteBatch || !m_loadingTexture)
        return;

    auto* deviceResources = GetUserResources()->GetDeviceResources();
    if (!deviceResources)
        return;

    // -------------------------------------------------
    // Screen size
    // -------------------------------------------------
    RECT outputRect = deviceResources->GetOutputSize();
    float screenWidth = float(outputRect.right - outputRect.left);
    float screenHeight = float(outputRect.bottom - outputRect.top);

    // -------------------------------------------------
    // Background texture size
    // -------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D11Resource> bgRes;
    m_loadingTexture->GetResource(bgRes.ReleaseAndGetAddressOf());

    Microsoft::WRL::ComPtr<ID3D11Texture2D> bgTex;
    if (FAILED(bgRes.As(&bgTex)))
        return;

    D3D11_TEXTURE2D_DESC bgDesc{};
    bgTex->GetDesc(&bgDesc);

    float bgWidth = float(bgDesc.Width);
    float bgHeight = float(bgDesc.Height);

    // -------------------------------------------------
    // Background scale (cover screen)
    // -------------------------------------------------
    float bgScaleX = screenWidth / bgWidth;
    float bgScaleY = screenHeight / bgHeight;
    float bgScale = std::max(bgScaleX, bgScaleY);

    Vector2 bgPos(
        (screenWidth - bgWidth * bgScale) * 0.5f,
        (screenHeight - bgHeight * bgScale) * 0.5f
    );

    Vector2 bgScaleVec(bgScale, bgScale);

    // -------------------------------------------------
    // Begin draw
    // -------------------------------------------------
    m_spriteBatch->Begin();

    // ---------------- Background ----------------
    m_spriteBatch->Draw(
        m_loadingTexture.Get(),
        bgPos,
        nullptr,
        Colors::White,
        0.0f,
        Vector2::Zero,
        bgScaleVec
    );

    // -------------------------------------------------
    // Loading bar (frame + fill)
    // -------------------------------------------------
    if (m_barFrameSRV && m_barFillSRV)
    {
        // --- Query bar texture size ---
        Microsoft::WRL::ComPtr<ID3D11Resource> barRes;
        m_barFrameSRV->GetResource(barRes.ReleaseAndGetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> barTex;
        if (SUCCEEDED(barRes.As(&barTex)))
        {
            D3D11_TEXTURE2D_DESC barDesc{};
            barTex->GetDesc(&barDesc);

            float barTexW = float(barDesc.Width);
            float barTexH = float(barDesc.Height);

            // Desired on-screen size
            const float barWidth = 320.0f;
            const float barHeight = 30.0f;

            Vector2 barScale(
                barWidth / barTexW,
                barHeight / barTexH
            );

            Vector2 barPos(
                (screenWidth - barWidth) * 0.5f,
                screenHeight * 0.75f
            );
            // ---- Fill ----
            RECT fillRect{};
            fillRect.left = 0;
            fillRect.top = 0;
            fillRect.right = static_cast<LONG>(barTexW * m_progress);
            fillRect.bottom = static_cast<LONG>(barTexH);


            // ---- Frame  on Top----
            m_spriteBatch->Draw(
                m_barFrameSRV.Get(),
                barPos,
                nullptr,
                Colors::White,
                0.0f,
                Vector2::Zero,
                barScale
            );

            m_spriteBatch->Draw(
                m_barFillSRV.Get(),
                barPos,
                &fillRect,
                Colors::White,
                0.0f,
                Vector2::Zero,
                barScale
            );
        }
    }

    // -------------------------------------------------
    // End draw
    // -------------------------------------------------
    m_spriteBatch->End();
}
/*--------------------------------------------------
終了処理
--------------------------------------------------*/
void LoadScreen::Finalize()
{
}

void LoadScreen::CreateDeviceDependentResources()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(GetUserResources()->GetDeviceResources()->GetD3DDeviceContext());
	//Load Texture
	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/LoadingBar.png", nullptr, m_barFillSRV.ReleaseAndGetAddressOf())
	);
	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/LoadingBarFill.png", nullptr, m_barFrameSRV.ReleaseAndGetAddressOf())
	);
}

void LoadScreen::CreateWindowSizeDependentResources()
{
    auto* dr = GetUserResources()->GetDeviceResources();
    RECT rc = dr->GetOutputSize();

    m_screenWidth = float(rc.right - rc.left);
    m_screenHeight = float(rc.bottom - rc.top);
}

void LoadScreen::OnDeviceLost()
{
	Finalize();
}

bool LoadScreen::IsReadyToFinish() const
{
    
    // Wait for loading to finish AND bar to visually reach 100%
    return m_loadingDone && m_progress >= 0.99f;
}

void LoadScreen::OnLoadingComplete()
{
    m_loadingDone = true;
}
