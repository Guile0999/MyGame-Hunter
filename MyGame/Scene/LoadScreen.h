#pragma once
#include "vector"
#include "SceneManager.h"
#include "UserResources.h"
class LoadScreen : public MyLib::LoadingScreen<UserResources>
{
private:
	//Sprite Assets
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_barFrameSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_barFillSRV;
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

	//For Create loadingTexture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_loadingTexture;

	 //seconds
	//For animated "Loading..." text;
	int m_loadingDotCount = 0;
	float m_dotTimer = 0.0f;
	const float m_dotInterval = 0.5f;

	float m_screenWidth = 0.0f;
	float m_screenHeight = 0.0f;

	//Timing
	float m_progress = 0.0f;
	
	float m_elapsed = 0.0f;
	bool m_loadingDone = false;
	static constexpr float MIN_DISPLAY_TIME = 0.5f;
public:
	// コンストラクタ
	LoadScreen();

	// デストラクタ
	~LoadScreen();

	// 初期化
	void Initialize() override;

	// 更新
	void Update(float elapsedTime) override;

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

	//
	bool IsReadyToFinish() const;

	void OnLoadingComplete() override;
};

