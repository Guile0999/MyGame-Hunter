#include <pch.h>
#include "Scope.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
	
	// コンストラクタ
	Scope::Scope()
		:
		m_state(ScopeStates::Inactive),                          // 現在の状態（非表示）
		m_spriteBatch(nullptr),                                  // スプライトバッチ
		m_scopeTexture(nullptr),                                 // スコープ画像
		m_whiteTexture(nullptr),                                 // 白テクスチャ
		m_zoom1(2.0f),                                           // ズーム倍率1
		m_zoom2(4.0f),                                           // ズーム倍率2
		m_breathTime(0.0f),                                      // 息を使った時間
		m_shakeAmount(9.5f),                                     // 揺れの大きさ
		m_currentShake(DirectX::SimpleMath::Vector2::Zero),      // 現在の揺れ
		m_shakeTime(0.0f),                                       // 揺れ用の時間
		m_holdingBreath(false),                                  // 息を止めているか
		m_resources(nullptr)                                     // ユーザーリソース
	{
	}
	
	// 初期化：スプライトバッチ・スコープ画像・ゲージ用テクスチャを用意
	void Scope::Initialize(UserResources* resources)
	{
		m_resources = resources;

		// SpriteBatch作成
		auto device = m_resources->GetDeviceResources()->GetD3DDevice();
		auto context = m_resources->GetDeviceResources()->GetD3DDeviceContext();

		m_spriteBatch = std::make_unique<SpriteBatch>(context);

		// スコープ画像を読み込む（DDS形式）
		DX::ThrowIfFailed(
			DirectX::CreateDDSTextureFromFile(
				device,
				L"Resources/Textures/Sniper/scopeGUI.dds",
				nullptr,
				m_scopeTexture.ReleaseAndGetAddressOf()
			)
		);

		// 1x1 白ピクセルテクスチャ（息ゲージ描画用）
		{
			uint32_t whitePixel = 0xFFFFFFFF;
			D3D11_TEXTURE2D_DESC d = {};
			d.Width = 1; d.Height = 1; d.MipLevels = 1; d.ArraySize = 1;
			d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			d.SampleDesc.Count = 1;
			d.Usage = D3D11_USAGE_IMMUTABLE;
			d.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &whitePixel;
			data.SysMemPitch = sizeof(uint32_t);

			Microsoft::WRL::ComPtr<ID3D11Texture2D> t;
			DX::ThrowIfFailed(device->CreateTexture2D(&d, &data, t.GetAddressOf()));
			DX::ThrowIfFailed(device->CreateShaderResourceView(
				t.Get(), nullptr, m_whiteTexture.ReleaseAndGetAddressOf()));
		}
	}

	
	// スコープ状態切り替え（非表示 → ズーム1 → ズーム2 → 非表示）
	void Scope::CycleState()
	{
		switch (m_state)
		{
		case ScopeStates::Inactive:  m_state = ScopeStates::Zoom1; break;
		case ScopeStates::Zoom1:  m_state = ScopeStates::Zoom2; break;
		case ScopeStates::Zoom2:  m_state = ScopeStates::Inactive; break;
		}
	}

	// 更新処理
	//  - 息を止めていない時：揺れる
	//  - 息を止めている時：安定する（息が続く間のみ）

	void Scope::Update(float elapsedTime, bool holdingBreath)
	{
		m_holdingBreath = holdingBreath;

		// スコープ非表示なら揺れと息をリセット
		if (!IsActive())
		{
			m_currentShake = Vector2::Zero;
			m_breathTime = 0.0f;
			return;
		}

		// 息タイマー：止めている間は消費、離すと回復
		if (m_holdingBreath && m_breathTime < BREATH_MAX)
		{
			m_breathTime += elapsedTime;   // 息を消費
			if (m_breathTime > BREATH_MAX) m_breathTime = BREATH_MAX;
		}
		else if (!m_holdingBreath && m_breathTime > 0.0f)
		{
			m_breathTime -= elapsedTime * BREATH_RECOVERY;  // 息を回復
			if (m_breathTime < 0.0f) m_breathTime = 0.0f;
		}

		// 安定できるのは「息を止めている」かつ「息が切れていない」時だけ
		bool steady = m_holdingBreath && (m_breathTime < BREATH_MAX);

		// 2軸の揺れを計算（異なる周波数で自然な揺れに）
		m_shakeTime += elapsedTime;
		Vector2 targetShake = Vector2::Zero;
		if (!steady)
		{
			targetShake.x = sinf(m_shakeTime * 1.3f) * m_shakeAmount;
			targetShake.y = sinf(m_shakeTime * 1.7f + 1.0f) * m_shakeAmount * 0.6f; // 縦は控えめ
		}

		// 目標値へ滑らかに補間（安定・復帰が急にならないように）
		float blend = std::min(1.0f, elapsedTime * 6.0f);
		m_currentShake += (targetShake - m_currentShake) * blend;
	}


	// 描画：スコープ画像 ＋ 息ゲージ
	void Scope::Render(const DirectX::SimpleMath::Vector2& screenSize)
	{
		// 非表示状態なら描画しない
		if (!IsActive())
			return;

		m_spriteBatch->Begin();

		// スコープを画面より少し大きく描き、揺れても端が見えないようにする
		float scopeWidth = screenSize.x * 1.1f;
		float scopeHeight = screenSize.y * 1.1f;

		// 中央配置＋揺れオフセットを適用
		RECT destRect;
		destRect.left = static_cast<LONG>((screenSize.x - scopeWidth) * 0.5f + m_currentShake.x);
		destRect.top = static_cast<LONG>((screenSize.y - scopeHeight) * 0.5f + m_currentShake.y);
		destRect.right = static_cast<LONG>(destRect.left + scopeWidth);
		destRect.bottom = static_cast<LONG>(destRect.top + scopeHeight);

		// スコープ画像を描画
		m_spriteBatch->Draw(m_scopeTexture.Get(), destRect);

		//息ゲージ（ブレスバー)
		if (m_whiteTexture)
		{
			float frac = m_breathTime / BREATH_MAX;     // 0=満タン, 1=使い切り
			float remaining = 1.0f - frac;              // 残量

			float barW = screenSize.x * 0.2f;           // バーの最大幅
			float barH = 10.0f;
			float barX = (screenSize.x - barW) * 0.5f;  // 中央
			float barY = screenSize.y * 0.8f;           // 下寄り

			// 背景（暗いバー）
			RECT bg;
			bg.left = (LONG)barX;
			bg.top = (LONG)barY;
			bg.right = (LONG)(barX + barW);
			bg.bottom = (LONG)(barY + barH);
			m_spriteBatch->Draw(m_whiteTexture.Get(), bg, nullptr,
				Color(0.1f, 0.1f, 0.1f, 0.6f));

			// 残量バー（緑→赤に変化）
			RECT fg = bg;
			fg.right = (LONG)(barX + barW * remaining);
			Color barColor = (remaining > 0.3f)
				? Color(0.3f, 1.0f, 0.3f, 0.9f)   // 緑（余裕あり）
				: Color(1.0f, 0.3f, 0.3f, 0.9f);  // 赤（残りわずか）
			m_spriteBatch->Draw(m_whiteTexture.Get(), fg, nullptr, barColor);
		}

		m_spriteBatch->End();
	}


	// 現在の状態に応じたズーム倍率を返す
	float Scope::GetZoomFactor() const
	{
		switch (m_state)
		{
		case ScopeStates::Zoom1: return m_zoom1; // ズーム1
		case ScopeStates::Zoom2: return m_zoom2; // ズーム2
		default: return 1.0f;                    // 非表示＝通常の視野
		}
	}

}