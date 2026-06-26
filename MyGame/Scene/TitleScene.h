#pragma once
#include "SceneManager.h"
#include "../UserResources.h"
class TitleScene : public MyLib::Scene<UserResources>
{
	// 初期化
	void Initialize() override;

	// 更新
	void Update(float elapsedTime)  override;

	// 描画
	void Render() override;

	// 終了処理
	void Finalize() override;

	// デバイスに依存するリソースを作成する関数
	void CreateDeviceDependentResources() override;

	// ウインドウサイズに依存するリソースを作成する関数
	void CreateWindowSizeDependentResources() override;

	// デバイスロストした時に呼び出される関数
	void OnDeviceLost() override;
private:
	//Create Texture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_titleTexture;
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	float m_fadeAlpha = 0.0f;
	int m_selectedIndex = 0;
	std::vector<std::wstring> m_menuItems = { L"Start Game",L"Options",L"Credits",L"Exit" };

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_playButtonTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_selectorTexture; // 1x1 white for other buttons
	std::unique_ptr<DirectX::SpriteFont> m_menuFont;
	float m_selectorAnim = 0.0f;
	float m_time = 0.0f;
	float m_menuFadeAlpha = 0.0f;
	
};

