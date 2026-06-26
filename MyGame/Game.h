//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "UserResources.h"
#include "MyLib/Utilities/DebugFont.h"
#include "Scene/SceneManager.h"
#include "Objects/FpsCamera.h"



// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // キーボードステートトラッカー
    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;

    // マウスステートステッカー
    DirectX::Mouse::ButtonStateTracker m_mouseTracker;

    // 共通ステートへのポインタ
    std::unique_ptr<DirectX::CommonStates> m_states;

    // デバッグ文字列表示オブジェクト
    std::unique_ptr<MyLib::DebugFont> m_debugFont;

    // シーンマネージャのポインタ
    std::unique_ptr<MyLib::SceneManager<UserResources>> m_sceneManager;

    // シーンへ渡すユーザ定義のリソースのポインタ
    std::unique_ptr<UserResources> m_userResources;

    

};
