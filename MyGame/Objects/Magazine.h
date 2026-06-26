//--------------------------------------------------------------------------------------
// File: Magazine.h
//
// 弾倉クラス
//
//-------------------------------------------------------------------------------------

#pragma once

#include "StepTimer.h"
#include "UserInterface.h"
#include <DeviceResources.h>
#include <SimpleMath.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <WICTextureLoader.h>
#include <CommonStates.h>
#include <vector>
#include "Keyboard.h"
#include <Audio.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>

namespace MyLib
{
	class Magazine
	{
	public:
		//	弾の状態
		enum STATE
		{
			IDOLE	= 0,
			SHOT	= 1	<< 0,
			RELOAD	= 1 << 1,
		};


	//	変数
	private:
		//	定数
		const static int   MAGAZINE_0_X;
		const static int   MAGAZINE_0_Y;
		const static int   MAGAZINE_MAX;
		const static float MAGAZINE_RANGE;


		DX::DeviceResources* m_pDR;

		std::vector<std::unique_ptr<MyLib::UserInterface>> m_bullets;

		int m_windowWidth, m_windowHeight;

		DirectX::Keyboard::KeyboardStateTracker m_tracker;

		byte m_magazineState;
		float m_move;

		int m_reloadWait;
		//	リロード関連の変数
		bool    m_isReloading   = false;
		float  m_reloadProgress = 0.0f;
		float  m_reloadDuration = 2.0f; // リロードにかかる時間（秒）
		bool   m_showComplete = false;  // リロード完了の表示フラグ
		float  m_completeTimer = 0.0f;  // リロード完了の表示タイマー

		std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
		std::unique_ptr<DirectX::SpriteFont> m_font;
		
		DirectX::AudioEngine* m_audioEngine = nullptr;
		std::unique_ptr<DirectX::SoundEffect> m_reloadSound;

		std::unique_ptr<DirectX::SoundEffectInstance> m_reloadInstance;

		float m_baseX = 0.0f;
		float m_baseY = 0.0f;

	//	関数
	public:
		Magazine();
		~Magazine();

		void Initialize(DX::DeviceResources* pDR, int width, int height);
		void Update(float elapsedTime);
		void Render();

		void Shoot();
		int GetAmmoCount() const;
		void SetWindowSize(int width, int height);


		void Add(const wchar_t* path
			, DirectX::SimpleMath::Vector2 position
			, DirectX::SimpleMath::Vector2 scale
			, MyLib::ANCHOR anchor);

		void Reload();

		void Move();
		//	セッター
		void SetAudioEngine(DirectX::AudioEngine* audioEngine) { m_audioEngine = audioEngine; }

	};
}