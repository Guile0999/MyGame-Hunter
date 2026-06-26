#pragma once
#include <pch.h>
#include "DeviceResources.h"
#include <Scene/SceneManager.h>
#include <MyLib/Utilities/DebugFont.h>
#include <UserResources.h>
#include <MyLib/Utilities/GridFloor.h>
#include <MyLib/Utilities/FloorPrimitive.h>
#include <MyLib/Utilities/RayCastSystem.h>
#include <MyLib/Utilities/Animation.h>
#include <MyLib/Utilities/BinaryFile.h>

// オブジェクト関連
#include <Objects/FpsCamera.h>
#include <Objects/FpsGun.h>
#include <Objects/FpsGunSetting.h>
#include <Objects/FirstPersonPlayer.h>
#include <Objects/FpsPlayerSetting.h>
#include <Objects/Scope.h>
#include <Objects/Bullet.h>
#include <Objects/Deer.h>
#include <Objects/BulletCamera.h>
#include <Objects/CameraManager.h>
#include <Objects/Mission.h>
#include <Objects/Boss.h>
#include <Objects/Rabbit.h>
// オーディオ
#include <Audio.h>


// ステージ関連
#include <Stage/Stage.h>
#include <Objects/Magazine.h>
#include <Scene/ResultScene.h>
#include <Stage/Levels.h>

namespace MyLib {

    // テスト用シーン（FPSゲームプレイ用）
    class TestScene : public Scene<UserResources>
    {
    public:
        TestScene();
        void Initialize() override;                 // 初期化
        void Update(float elapsedTime) override;    // 更新
        void Render() override;                     // 描画
        void Finalize() override;                   // 終了処理
        void CreateDeviceDependentResources() override;       // デバイス依存リソース作成
        void CreateWindowSizeDependentResources() override;   // ウィンドウサイズ依存リソース作成
        void OnDeviceLost() override;                          // デバイス喪失時
      
    private:

        // 発射処理
        void UpdateFiring(); 

        // 弾の更新＋命中判定
        void UpdateBullets(float elapsedTime); 

        // ボス出現・更新・ダメージ
        void UpdateBoss(float elapsedTime);  

        // 完了・シーン遷移チェック
        void UpdateCompletion();  

        // ビュー／射影行列の決定
        void UpdateCameraMatrices(); 

        // マズルフラッシュ・弾痕などのタイマ
        void UpdateTimers(float elapsedTime);  

        // スコープ更新・切替
        void UpdateScope(float elapsedTime, bool bulletCamActive);  

        // バレットカメラ更新
        void UpdateBulletCamera(float elapsedTime); 

        // 開始数フレームのカーソル非表示
        void HideCursorFirstFrames(); 

        // スコアパネル中ならtrueを返す（更新を止める）
        bool HandleScorePanel();            

    private:
        //オーディオ
        std::vector<std::unique_ptr<DirectX::SoundEffectInstance>> m_activeSoundInstances;
        std::unique_ptr<DirectX::AudioEngine> m_audioEngine;
        std::unique_ptr<DirectX::SoundEffect> m_shootSound;

        //  環境音（ループ再生）
        std::unique_ptr<DirectX::SoundEffect> m_windSound;
        std::unique_ptr<DirectX::SoundEffect> m_birdsSound;
        std::unique_ptr<DirectX::SoundEffect> m_cicadasSound;
        std::unique_ptr<DirectX::SoundEffectInstance> m_windInstance;
        std::unique_ptr<DirectX::SoundEffectInstance> m_birdsInstance;
        std::unique_ptr<DirectX::SoundEffectInstance> m_cicadasInstance;

        // --- デバイスリソース ---
        std::unique_ptr<DX::DeviceResources> m_devicesResources;

        // --- カメラと描画 ---
        DirectX::SimpleMath::Matrix m_view;       // ビュー行列
        DirectX::SimpleMath::Matrix m_proj;       // 射影行列
        std::unique_ptr<DirectX::BasicEffect> m_basicEffect;
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primitiveBatch;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

        // --- 床 ---
        std::unique_ptr<MyLib::FloorPrimitive> m_floor;

        // --- プレイヤー ---
        std::unique_ptr<FirstPersonPlayer> m_player;

        // --- ガン ---
        std::unique_ptr<FpsGun> m_fpsGun;
        std::unique_ptr<DirectX::Model> m_fpsGunModel;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_PS_Gun;

      
        // --- 鹿ターゲット ---
        std::vector<std::unique_ptr<Deer>> m_deers;
		// --- ウサギターゲット ---
        std::vector<std::unique_ptr<Rabbit>> m_rabbits;
        //ボス
        std::unique_ptr<Boss> m_boss;
        bool  m_bossSpawned;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bossWarningTexture;
        bool  m_showBossWarning;
        float m_bossWarningTimer;
        static constexpr float BOSS_WARNING_DURATION = 3.0f;
      
        
        // 弾 
        // 弾モデル（
        std::shared_ptr<DirectX::Model> m_bulletModel;
        // 弾のプール（TestSceneが所有・再利用する）
        std::vector<std::unique_ptr<Bullet>> m_bulletPool;
        static constexpr int BULLET_POOL_SIZE = 64;
        // バレットカメラが追従中の弾（観測のみ・所有しない）
        Bullet* m_bulletCamTarget;

        // --- スコープ ---
        std::unique_ptr<Scope> m_scope;
        bool m_showScopeText;            
        std::wstring m_scopeDebugText;
         float m_scopeTextTimer;

        // --- ヒットメッセージ ---
        float m_hitMessageTimer;

       //CursorHideFrame
        int m_cursorHideFrames;
        // --- UI ---
        std::unique_ptr<DebugFont> m_debugFont;
        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

        // muzzle flash UI
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_muzzleFlashTexture;
        float m_muzzleFlashTimer;
        int   m_muzzleFlashFrame ;   // 0 = top frame, 1 = bottom frame
        bool  m_showMuzzleFlash;  
        static constexpr float MUZZLE_FLASH_DURATION = 0.07f;
       

        // --- ミッション UI ---
        std::unique_ptr<Mission> m_mission;
        Levels m_levels;
        int    m_killsNeeded ;
        int    m_killsGot;

        // --- Score panel overlay ---
        bool m_showScorePanel;   // mission over, panel showing over frozen scene
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scorePanelTexture; // Big.png
        std::unique_ptr<DirectX::SpriteFont> m_scoreFont;

        // --- Blood splat effect ---
       
        bool    m_showBulletHole;
        Vector3 m_bulletHolePos = Vector3::Zero;
        float   m_bulletHoleTimer;
        float   m_bulletHoleAlpha;
        static constexpr float BULLET_HOLE_DURATION = 5.0f;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bulletHoleTexture;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_damageTexture;
        bool  m_showDamageEffect;
        float m_damageEffectTimer;
        static constexpr float DAMAGE_EFFECT_DURATION = 1.5f;   // show for 1.5s before resul

        // --- サーマルビジョン ---
        bool m_thermalEnabled;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_fullscreenVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_thermalPS;
      

        // --- 内部ヘルパー ---
        void DrawSceneObjects();
        Bullet* GetUnusedBullet();   // プールから未使用の弾を取得
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_thermalGlowTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scopeOverlayTexture;

        // --- クロスヘア ---
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_crosshairTexture;
        DirectX::SimpleMath::Vector2 m_crosshairOrigin;

        // --- レイキャスター ---
        std::shared_ptr<RaycastSystem> m_rayCaster;

        // --- 入力 ---
        DirectX::Mouse::ButtonStateTracker m_mouseTracker;

        // --- マガジン UI ---
        std::unique_ptr<MyLib::Magazine> m_mag;

        // --- ステージ ---
        std::unique_ptr<MyLib::Stage> m_stage;

		
        // --- Bolt action animation ---
        bool  m_boltAnimating;
        float m_boltTimer;
        static constexpr float BOLT_DURATION = 0.4f; // seconds

      
    };

} // namespace MyLib