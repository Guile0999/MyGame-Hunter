//--------------------------------------------------------------------------------------
// File: Magazine.cpp
//
// 弾倉クラス
//
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "Magazine.h"
#include "UserInterface.h"

#include "MyLib/Utilities/BinaryFile.h"
#include "DeviceResources.h"
#include <SimpleMath.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <WICTextureLoader.h>
#include <CommonStates.h>
#include <vector>

using namespace DirectX;

//  表示弾数（増えると上にのびる）
const  int      MyLib::Magazine::MAGAZINE_MAX = 6;
//  弾の表示間隔
const  float    MyLib::Magazine::MAGAZINE_RANGE = 30.0f;

// 旧固定座標は廃止し、ウインドウサイズに比例した m_baseX / m_baseY を使う

/// <summary>
/// コンストラクタ
/// </summary>
MyLib::Magazine::Magazine()
    : m_windowHeight(0)
    , m_windowWidth(0)
    , m_pDR(nullptr)
    , m_magazineState(STATE::IDOLE)
    , m_move(0.0f)
    , m_reloadWait(0)
{
    m_bullets.clear();
}

/// <summary>
/// デストラクタ
/// </summary>
MyLib::Magazine::~Magazine()
{
}

/// <summary>
/// 初期化
/// </summary>
void MyLib::Magazine::Initialize(DX::DeviceResources* pDR, int width, int height)
{
    m_pDR = pDR;
    m_windowWidth = width;
    m_windowHeight = height;

    // ウインドウサイズに比例した表示位置（右下付近）
    m_baseX = m_windowWidth * 0.96f;   // 横96%＝右端付近
    m_baseY = m_windowHeight * 0.83f;   // 縦83%＝下付近

    //  最初は最大弾数分補充する
    for (int i = 0; i < MAGAZINE_MAX; i++)
    {
        Add(L"Resources/Textures/Bullet.png"
            , DirectX::SimpleMath::Vector2(m_baseX, m_baseY - MAGAZINE_RANGE * i)
            , DirectX::SimpleMath::Vector2(0.5f, 0.5f)
            , MyLib::ANCHOR::BOTTOM_RIGHT);
    }

    auto device = m_pDR->GetD3DDevice();
    auto context = m_pDR->GetD3DDeviceContext();

    // リロード表示用のスプライトバッチ・フォント
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
    m_font = std::make_unique<DirectX::SpriteFont>(device, L"Resources/Font/SegoeUI_18.spritefont");

    // リロード音の読み込み
    if (m_audioEngine)
    {
        m_reloadSound = std::make_unique<SoundEffect>(
            m_audioEngine, L"Resources/Sounds/reload.wav");
    }
}

/// <summary>
/// 更新
/// </summary>
void MyLib::Magazine::Update(float elapsedTime)
{
    //  キーボード情報の更新
    DirectX::Keyboard::State keystate = DirectX::Keyboard::Get().GetState();
    m_tracker.Update(keystate);

    // Rキーでリロード開始
    if (m_tracker.pressed.R && !m_isReloading && m_bullets.size() < MAGAZINE_MAX)
    {
        m_magazineState |= STATE::RELOAD;
        m_isReloading = true;
        m_reloadProgress = 0.0f;
        m_showComplete = false;

        // リロード音を再生
        if (m_reloadSound)
        {
            auto inst = m_reloadSound->CreateInstance();
            inst->Play();
            m_reloadInstance = std::move(inst);
        }
    }

    // 発射中なら弾を移動させる
    if (m_magazineState & STATE::SHOT)
        Move();

    // リロード進行度を進める
    if (m_isReloading)
    {
        m_reloadProgress += elapsedTime / m_reloadDuration;
        if (m_reloadProgress >= 1.0f)
        {
            m_reloadProgress = 1.0f;
            m_isReloading = false;
            m_showComplete = true;
            m_completeTimer = 0.0f;

            // 弾を最大数まで補充
            while ((int)m_bullets.size() < MAGAZINE_MAX)
            {
                Add(L"Resources/Textures/Bullet.png",
                    DirectX::SimpleMath::Vector2(m_baseX, m_baseY - MAGAZINE_RANGE * m_bullets.size()),
                    DirectX::SimpleMath::Vector2(0.5f, 0.5f),
                    MyLib::ANCHOR::BOTTOM_RIGHT);
            }
            m_magazineState &= ~STATE::RELOAD;
        }
    }

    // 「Reload Complete」表示タイマー
    if (m_showComplete)
    {
        m_completeTimer += elapsedTime;
        if (m_completeTimer > 1.0f)
            m_showComplete = false;
    }
}

/// <summary>
/// 描画
/// </summary>
void MyLib::Magazine::Render()
{
    //  残弾０なら表示しない
    if (m_bullets.empty())
    {
        return;
    }
    //  発射中じゃないときは装填弾も表示する
    if (!(m_magazineState & STATE::SHOT))
    {
        m_bullets[0]->Render();
    }
    //  装填弾以外を描画する
    for (int i = 1; i < m_bullets.size(); i++)
    {
        m_bullets[i]->Render();
    }

    // リロード中／完了表示
    if (m_isReloading || m_showComplete)
    {
        m_spriteBatch->Begin();

        float cx = m_windowWidth * 0.5f;
        float cy = m_windowHeight * 0.65f;

        if (m_isReloading)
        {
            // 「Reloading...」テキスト
            const wchar_t* txt = L"Reloading...";
            XMVECTOR sz = m_font->MeasureString(txt);
            float w = XMVectorGetX(sz);
            m_font->DrawString(m_spriteBatch.get(), txt,
                SimpleMath::Vector2(cx - w * 0.5f, cy - 50.0f),
                Colors::White);

            // リロード進行度をパーセント表示
            wchar_t pct[32];
            swprintf_s(pct, L"%d%%", (int)(m_reloadProgress * 100.0f));
            XMVECTOR psz = m_font->MeasureString(pct);
            float pw = XMVectorGetX(psz);
            m_font->DrawString(m_spriteBatch.get(), pct,
                SimpleMath::Vector2(cx - pw * 0.5f, cy),
                Colors::Yellow, 0.0f, SimpleMath::Vector2::Zero, 1.5f);
        }
        else if (m_showComplete)
        {
            const wchar_t* txt = L"Reload Complete";
            XMVECTOR sz = m_font->MeasureString(txt);
            float w = XMVectorGetX(sz);
            m_font->DrawString(m_spriteBatch.get(), txt,
                SimpleMath::Vector2(cx - w * 0.5f, cy - 50.0f),
                Colors::LightGreen);
        }

        m_spriteBatch->End();
    }
}

/// <summary>
/// 発射フラグを立てる
/// </summary>
void MyLib::Magazine::Shoot()
{
    //  弾があれば発射状態にする
    if (!m_bullets.empty())
    {
        if (!(m_magazineState & STATE::SHOT))
        {
            m_magazineState |= STATE::SHOT;
        }
    }
}

/// <summary>
/// 残弾数を返す
/// </summary>
int MyLib::Magazine::GetAmmoCount() const
{
    return static_cast<int>(m_bullets.size());
}

/// <summary>
/// ウインドウサイズをセットする（リサイズ時に位置を再計算）
/// </summary>
void MyLib::Magazine::SetWindowSize(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;

    // ウインドウサイズに合わせて表示位置を再計算
    m_baseX = m_windowWidth * 0.96f;
    m_baseY = m_windowHeight * 0.83f;

    // 既存のUI要素にもウインドウサイズを反映
    for (auto& bulletUI : m_bullets)
    {
        bulletUI->SetWindowSize(width, height);
    }
}

/// <summary>
/// 追加関数
/// </summary>
void MyLib::Magazine::Add(const wchar_t* path, DirectX::SimpleMath::Vector2 position, DirectX::SimpleMath::Vector2 scale, MyLib::ANCHOR anchor)
{
    std::unique_ptr<MyLib::UserInterface> userInterface = std::make_unique<MyLib::UserInterface>();
    userInterface->Create(m_pDR
        , path
        , position
        , scale
        , anchor);
    userInterface->SetWindowSize(m_windowWidth, m_windowHeight);

    m_bullets.push_back(std::move(userInterface));
}

/// <summary>
/// リロード処理（旧フレームベース。現在は未使用だが残す）
/// </summary>
void MyLib::Magazine::Reload()
{
    //  弾がMAXならリロード解除
    if (m_bullets.size() == MAGAZINE_MAX)
    {
        m_magazineState ^= STATE::RELOAD;
        return;
    }

    //  3フレームごとに弾を追加
    if (m_reloadWait == 0)
    {
        Add(L"Resources/Textures/Bullet.png"
            , DirectX::SimpleMath::Vector2(m_baseX, m_baseY - MAGAZINE_RANGE * m_bullets.size())
            , DirectX::SimpleMath::Vector2(0.5f, 0.5f)
            , MyLib::ANCHOR::BOTTOM_RIGHT);
        m_reloadWait = 3;
    }
    m_reloadWait--;
}

/// <summary>
/// 弾の移動処理（発射時に下へスライド）
/// </summary>
void MyLib::Magazine::Move()
{
    //  毎フレーム移動させる
    m_move += 1.0f;

    //  1発分移動したら残弾を1減らして発射状態を終了
    if (m_bullets[0]->GetPosition().y > m_baseY + MAGAZINE_RANGE)
    {
        m_bullets.pop_back();
        m_move = 0;
        m_magazineState ^= STATE::SHOT;
    }

    //  移動量を反映して表示位置を更新
    for (int i = 0; i < static_cast<int>(m_bullets.size()); i++)
    {
        DirectX::SimpleMath::Vector2 pos = SimpleMath::Vector2(m_baseX, m_baseY - MAGAZINE_RANGE * i);
        pos.y += m_move;
        m_bullets[i]->SetPosition(pos);
    }
}