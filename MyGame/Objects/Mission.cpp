#include "pch.h"
#include "Objects/Mission.h"
#include <WICTextureLoader.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    //// コンストラクタ：各メンバを初期化子リストで初期化
    Mission::Mission()
        :
        m_briefingTexture(nullptr),                              // ブリーフィング画像
        m_panelTexture(nullptr),                                 // 隅パネル画像
        m_font(nullptr),                                         // フォント
        m_phase(Phase::Briefing),                                // 表示フェーズ
        m_phaseTimer(0.0f),                                      // フェーズタイマー
        m_fadeAlpha(1.0f),                                       // フェード用アルファ
        m_panelPos(DirectX::SimpleMath::Vector2::Zero),          // パネル位置
        m_panelScale(DirectX::SimpleMath::Vector2::Zero),        // パネルスケール
        m_screenW(1280.0f),                                      // 画面幅
        m_screenH(720.0f),                                       // 画面高さ
        m_briefingDone(false),                                   // ブリーフィング完了
        m_result(UserResources::MissionResult::None),            // ミッション結果
        m_anyHit(false),                                         // 1回でも命中したか
        m_checked(false),                                        // 結果処理済みか
        m_hitMessage(L""),                                       // 命中メッセージ
        m_deerLeft(0),                                           // 鹿の残り
        m_deerTotal(0),                                          // 鹿の総数
        m_rabbitLeft(0),                                         // ウサギの残り
        m_rabbitTotal(0),                                        // ウサギの総数
        m_trackBoss(false),                                      // ボスを追跡表示するか
        m_bossAlive(false),                                      // ボス生存中か
        m_lastMissTextIndex(-1),                                 // 直近のMissテキスト位置
        m_missTextPending(false),                                // Miss表示保留中か
        m_missDisplayTimer(0.0f),                                // Miss表示タイマー
        m_missTextShowing(false),                                // Miss表示中か
        m_timeLeft(60.0f),                                       // 残り時間
        m_missCount(0),                                          // ミス回数
        m_numberTexture(nullptr),                                // 数字画像
        m_stageTexture(nullptr),                                 // ステージバナー画像
        m_stage(1),                                              // ステージ番号
        m_missionObjective(L""),                                 // 目標文
        m_stageBannerTimer(0.0f),                                // バナータイマー
        m_stageBannerAlpha(1.0f),                                // バナーアルファ
        m_score(0),                                              // 合計スコア
        m_floatingTexts{},                                       // 浮きテキスト
        m_centerX(640.0f),                                       // 中央X
        m_centerY(360.0f),                                       // 中央Y
        m_userResources(nullptr),                                // ユーザーリソース
        m_earned(0),                                             // 獲得点
        m_penalty(0)                                             // 減点
    {
    }
    // 初期化
    void Mission::Initialize(UserResources* userResources)
    {
        m_userResources = userResources;

        auto device = userResources->GetDeviceResources()->GetD3DDevice();
        auto outRect = userResources->GetDeviceResources()->GetOutputSize();
        m_screenW = float(outRect.right);
        m_screenH = float(outRect.bottom);

        // ブリーフィング画面の画像を読み込む
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(
                device,
                L"Resources/Textures/mission panel.png",
                nullptr,
                m_briefingTexture.ReleaseAndGetAddressOf()
            )
        );

        // 隅に表示するミッションパネルを読み込む
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(
                device,
                L"Resources/Textures/Big.png",
                nullptr,
                m_panelTexture.ReleaseAndGetAddressOf()
            )
        );

        // タイマー用の数字画像（0-9）を読み込む
        DX::ThrowIfFailed(
            CreateDDSTextureFromFile(
                device,
                L"Resources/Textures/number.dds",
                nullptr,
                m_numberTexture.ReleaseAndGetAddressOf()
            )
        );

        // フォント読み込み
        m_font = std::make_unique<SpriteFont>(
            device,
            L"Resources/Font/SegoeUI_18.spritefont"
        );

        // ステージバナー画像を読み込む
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device,
                L"Resources/Textures/Stages.png",
                nullptr, m_stageTexture.ReleaseAndGetAddressOf()));

        // Big.pngは画面上の外から開始（後でスライドインする）
        m_panelScale = Vector2(SCALE_CORNER, SCALE_CORNER);
        m_panelPos = Vector2(10.0f, -PANEL_H * SCALE_CORNER); // 画面外（上）

        // 状態を初期化
        m_phase = Phase::Briefing;
        m_phaseTimer = 0.0f;
        m_fadeAlpha = 1.0f;
        m_briefingDone = false;
        m_result = UserResources::MissionResult::None;
        m_anyHit = false;
        m_checked = false;
        m_hitMessage = L"";

        // スコアUIの初期化
        m_centerX = m_screenW * 0.5f;
        m_centerY = m_screenH * 0.4f;  // 画面中央より少し上
        m_score = 0;
        m_floatingTexts.clear();

        // タイマー・ミス数の初期化
        m_timeLeft = START_TIME;
        m_missCount = 0;

        m_earned = 0;
        m_penalty = 0;
    }

    // 更新
    void Mission::Update(float elapsedTime, bool anyKeyPressed)
    {
        m_phaseTimer += elapsedTime;

        switch (m_phase)
        {
        case Phase::Briefing:
            // 何かキー/クリックされたら次へ
            if (anyKeyPressed)
            {
                m_phase = Phase::FadeOut;
                m_phaseTimer = 0.0f;
                m_fadeAlpha = 1.0f;
            }
            break;

        case Phase::FadeOut:
            // ブリーフィング画面をフェードアウト
            m_fadeAlpha = 1.0f - (m_phaseTimer / TIME_FADEOUT);
            if (m_phaseTimer >= TIME_FADEOUT)
            {
                m_fadeAlpha = 0.0f;
                m_briefingDone = true;   // これ以降プレイヤーが撃てる
                m_phase = Phase::SlideIn;
                m_phaseTimer = 0.0f;
            }
            break;

        case Phase::SlideIn:
        {
            // Big.pngを上から左上の隅へスライド
            float t = m_phaseTimer / TIME_SLIDE;
            t = std::min(t, 1.0f);
            float ease = Ease(t);

            float startY = -PANEL_H * SCALE_CORNER;    // 画面外（上）
            float endY = 10.0f;                        // 最終の隅位置

            m_panelPos = Vector2(10.0f, startY + (endY - startY) * ease);

            if (t >= 1.0f)
            {
                m_panelPos = Vector2(10.0f, 10.0f);
                m_phase = Phase::Corner;
            }
            break;
        }

        case Phase::Corner:
            // 隅に固定（アニメ無し）
            break;
        }

        // カウントダウンタイマー（プレイ中のみ）
        if (m_briefingDone && m_result == UserResources::MissionResult::None)
        {
            m_timeLeft -= elapsedTime * DrainSpeed();
            if (m_timeLeft <= 0.0f)
            {
                m_timeLeft = 0.0f;
                m_result = UserResources::MissionResult::Failed;   // 時間切れ＝失敗
            }
        }

        // ステージバナーのフェード
        if (m_briefingDone && m_stageBannerAlpha > 0.0f)
        {
            m_stageBannerTimer += elapsedTime;
            if (m_stageBannerTimer > STAGE_BANNER_HOLD)
            {
                float t = (m_stageBannerTimer - STAGE_BANNER_HOLD) / STAGE_BANNER_FADE;
                m_stageBannerAlpha = 1.0f - t;
                if (m_stageBannerAlpha < 0.0f) m_stageBannerAlpha = 0.0f;
            }
        }

        // ミス表示タイマー
        if (m_missTextShowing &&
            m_result == UserResources::MissionResult::None)
        {
            m_missDisplayTimer += elapsedTime;
            if (m_missDisplayTimer >= MISS_DISPLAY_DURATION)
            {
                m_result = UserResources::MissionResult::Failed;
                m_missTextShowing = false;
            }
        }

        // スコア用の浮き上がるテキストを更新
        for (auto& ft : m_floatingTexts)
        {
            ft.timer += elapsedTime;
            ft.pos.y -= 40.0f * elapsedTime; // 上へ移動
        }

        // 表示時間が終わったテキストを削除
        m_floatingTexts.erase(
            std::remove_if(m_floatingTexts.begin(), m_floatingTexts.end(),
                [](const FloatingText& ft) { return ft.timer >= ft.duration; }),
            m_floatingTexts.end()
        );

    }

    // 描画
    void Mission::Render(SpriteBatch* spriteBatch, bool bulletCamActive)
    {
        if (bulletCamActive) return;

        // ブリーフィング画面を中央に描画
        if (m_phase == Phase::Briefing || m_phase == Phase::FadeOut)
        {
            if (m_briefingTexture && m_fadeAlpha > 0.0f)
            {
                Vector2 briefPos(
                    (m_screenW - BRIEF_W * BRIEF_SCALE) * 0.5f,
                    (m_screenH - BRIEF_H * BRIEF_SCALE) * 0.5f
                );

                spriteBatch->Begin();
                spriteBatch->Draw(
                    m_briefingTexture.Get(),
                    briefPos,
                    nullptr,
                    Color(1.0f, 1.0f, 1.0f, m_fadeAlpha),
                    0.0f,
                    Vector2::Zero,
                    Vector2(BRIEF_SCALE, BRIEF_SCALE)
                );
                spriteBatch->End();
            }
        }
        else
        {
            // 隅のBig.pngパネル＋目標トラッカーを描画
            if (!m_panelTexture || !m_font) return;

            float s = SCALE_CORNER;

            // パネル背景を描画
            spriteBatch->Begin();
            spriteBatch->Draw(
                m_panelTexture.Get(),
                m_panelPos,
                nullptr,
                Color(1.0f, 1.0f, 1.0f, 0.6f),
                0.0f,
                Vector2::Zero,
                Vector2(s, s)
            );
            spriteBatch->End();

            spriteBatch->Begin();

            // レイアウト用の定数
            float padX = 30.0f * s;
            float padY = 25.0f * s;
            float headerSize = s * 2.0f;
            float bodySize = s * 1.5f;
            float lineH = m_font->GetLineSpacing() * bodySize;
            float headerGap = lineH + 45.0f * s;
            float rowGap = lineH + 9.0f * s;

            // 「MISSION」ヘッダー
            Vector2 headerPos(m_panelPos.x + padX, m_panelPos.y + padY);
            m_font->DrawString(
                spriteBatch, L"MISSION",
                headerPos,
                Color(1.0f, 0.82f, 0.2f, 1.0f),
                0.0f, Vector2::Zero, headerSize
            );

            // ライブ目標トラッカー（残り/総数）
            float trackY = headerPos.y + headerGap * 0.6f;
            wchar_t line[64];

            // 鹿の行（0になると緑）
            if (m_deerTotal > 0)
            {
                swprintf_s(line, L"Deer  (%d/%d)", m_deerLeft, m_deerTotal);
                Color c = (m_deerLeft == 0) ? Color(0.4f, 1.0f, 0.4f, 1.0f) : Color(1.0f, 1.0f, 1.0f, 1.0f);
                m_font->DrawString(spriteBatch, line,
                    Vector2(headerPos.x, trackY),
                    c, 0.0f, Vector2::Zero, bodySize);
                trackY += lineH;
            }
            // ウサギの行（0になると緑）
            if (m_rabbitTotal > 0)
            {
                swprintf_s(line, L"Rabbit (%d/%d)", m_rabbitLeft, m_rabbitTotal);
                Color c = (m_rabbitLeft == 0) ? Color(0.4f, 1.0f, 0.4f, 1.0f) : Color(1.0f, 1.0f, 1.0f, 1.0f);
                m_font->DrawString(spriteBatch, line,
                    Vector2(headerPos.x, trackY),
                    c, 0.0f, Vector2::Zero, bodySize);
                trackY += lineH;
            }

            // ボスの行（ステージ3で出現後のみ表示）
            if (m_trackBoss)
            {
                swprintf_s(line, L"Boss  (%d/1)", m_bossAlive ? 1 : 0);
                Color c = (!m_bossAlive) ? Color(0.4f, 1.0f, 0.4f, 1.0f) : Color(1.0f, 0.4f, 0.4f, 1.0f);
                m_font->DrawString(spriteBatch, line,
                    Vector2(headerPos.x, trackY),
                    c, 0.0f, Vector2::Zero, bodySize);
                trackY += lineH;
            }

            spriteBatch->End();

            // プレイ中はタイマーとステージバナーを描画
            if (!bulletCamActive)
            {
                DrawTimer(spriteBatch);
                DrawStageBanner(spriteBatch);
            }
        }

        // 浮きテキストはフェーズに関係なく常に描画
        if (!m_floatingTexts.empty() && m_font)
        {
            spriteBatch->Begin();

            for (const auto& ft : m_floatingTexts)
            {
                // 経過に応じて薄く・大きくする
                float alpha = 1.0f - (ft.timer / ft.duration);
                float scale = 1.5f + (ft.timer / ft.duration) * 0.3f;

                XMVECTOR size = m_font->MeasureString(ft.text.c_str());
                float textW = XMVectorGetX(size) * scale;

                Vector2 drawPos(ft.pos.x - textW * 0.5f, ft.pos.y);

                Color color(
                    ft.color.f[0],
                    ft.color.f[1],
                    ft.color.f[2],
                    alpha
                );

                m_font->DrawString(
                    spriteBatch,
                    ft.text.c_str(),
                    drawPos,
                    color,
                    0.0f,
                    Vector2::Zero,
                    scale
                );
            }

            spriteBatch->End();
        }
    }

    // 命中処理
    void Mission::OnHit(IRaycastTarget::HitPart hitPart)
    {
        if (m_result != UserResources::MissionResult::None) return;
        m_anyHit = true;

        switch (hitPart)
        {
        case IRaycastTarget::HitPart::HEAD:
            // ヘッドショット：+900、時間ボーナス
            m_hitMessage = L"Headshot!";
            m_score += 900;
            m_earned += 900;
            m_timeLeft += HEADSHOT_BONUS;

            // 「+900」の浮きテキストを出す
            m_floatingTexts.push_back({
                L"+900",
                DirectX::SimpleMath::Vector2(m_centerX, m_centerY),
                0.0f, 1.8f,
                DirectX::Colors::Gold
                });
            break;

        case IRaycastTarget::HitPart::BODY:
            // 胴体（急所）：+300
            m_hitMessage = L"Vital Organ Hit!";
            m_score += 300;
            m_earned += 300;

            m_floatingTexts.push_back({
                L"+300",
                DirectX::SimpleMath::Vector2(m_centerX, m_centerY),
                0.0f, 1.8f,
                DirectX::Colors::Gold
                });
            break;
        case IRaycastTarget::HitPart::LUNG:
            // 肺：+300
            m_hitMessage = L"Lung Shot!";
            m_score += 300;
            m_earned += 300;
            m_floatingTexts.push_back({
                L"+600",
                DirectX::SimpleMath::Vector2(m_centerX, m_centerY),
                0.0f, 1.8f,
                DirectX::Colors::Gold
                });
            break;

        default:
            m_hitMessage = L"Hit!";
            break;
        }
    }

    // ミス処理
    void Mission::OnMiss()
    {
        if (m_result != UserResources::MissionResult::None) return;

        m_missCount++;                       // ミスを数える（タイマーが速くなる）
        m_score -= 200;                      // ミスの減点
        m_penalty += 200;                    // 減点の累計
        if (m_missCount >= MAX_MISSES)       // 6回ミスで失敗
            m_result = UserResources::MissionResult::Failed;
    }

    // 発射時
    void Mission::OnBulletFired()
    {
        m_floatingTexts.push_back({
            L"Miss!",
            DirectX::SimpleMath::Vector2(m_centerX, m_centerY),
            0.0f, 1.8f,
            DirectX::Colors::Red
            });
        // 後で取り消せるようにインデックスを保存
        m_lastMissTextIndex = static_cast<int>(m_floatingTexts.size() - 1);
        m_missTextPending = true;
    }

    // 命中時
    void Mission::CancelMissText()
    {
        if (m_missTextPending && m_lastMissTextIndex >= 0 &&
            m_lastMissTextIndex < static_cast<int>(m_floatingTexts.size()))
        {
            m_floatingTexts.erase(m_floatingTexts.begin() + m_lastMissTextIndex);
            m_lastMissTextIndex = -1;
        }
        m_missTextPending = false;
    }

    // ボス命中
    void Mission::OnBossHit()
    {
        m_anyHit = true;
    }

    // ステージバナー描画
    void Mission::DrawStageBanner(DirectX::SpriteBatch* spriteBatch)
    {
        if (!m_stageTexture || !m_briefingDone) return;
        if (m_stageBannerAlpha <= 0.0f) return;

        // ステージN：行=(N-1)/3、列=(N-1)%3、各セル512x205
        int idx = m_stage - 1;
        int col = idx % 3;
        int row = idx / 3;

        RECT src;
        src.left = col * STAGE_CELL_W;
        src.top = row * STAGE_CELL_H;
        src.right = src.left + STAGE_CELL_W;
        src.bottom = src.top + STAGE_CELL_H;

        float scale = 0.8f;
        float bw = STAGE_CELL_W * scale;
        float bh = STAGE_CELL_H * scale;

        // 画面中央・タイマーの少し下に表示
        Vector2 pos((m_screenW - bw) * 0.5f, 70.0f);

        spriteBatch->Begin(SpriteSortMode_Deferred,
            m_userResources->GetCommonStates()->NonPremultiplied());
        spriteBatch->Draw(m_stageTexture.Get(), pos, &src,
            Color(1.0f, 1.0f, 1.0f, m_stageBannerAlpha),   // アルファでフェード
            0.0f, Vector2::Zero, Vector2(scale, scale));
        spriteBatch->End();
    }

    // 強制的に成功にする（キル条件達成時に呼ぶ）
    void Mission::ForceSuccess()
    {
        if (m_result == UserResources::MissionResult::None)
            m_result = UserResources::MissionResult::Success;
    }

    void Mission::ForceFail()
    {
        if (m_result == UserResources::MissionResult::None)
            m_result = UserResources::MissionResult::Failed;
    }

    // 目標トラッカーの数値を更新（毎フレームTestSceneから呼ばれる）
    void Mission::SetObjectiveCounts(int deerLeft, int deerTotal, int rabbitLeft, int rabbitTotal, bool hasBoss, bool bossAlive)
    {
        m_deerLeft = deerLeft;     m_deerTotal = deerTotal;
        m_rabbitLeft = rabbitLeft; m_rabbitTotal = rabbitTotal;
        m_trackBoss = hasBoss;     m_bossAlive = bossAlive;
    }

    // タイマー描画
    void Mission::DrawTimer(DirectX::SpriteBatch* spriteBatch)
    {
        if (!m_numberTexture || !m_briefingDone) return;

        // 残り秒数（切り上げ）
        int seconds = (int)std::ceil(m_timeLeft);
        if (seconds < 0) seconds = 0;

        // 数字文字列を作る（例：60, 9, 0）
        wchar_t buf[8];
        swprintf_s(buf, L"%d", seconds);
        int numDigits = (int)wcslen(buf);

        float scale = 2.0f;  // 画面上の数字サイズ
        float dw = DIGIT_W * scale;
        float dh = DIGIT_H * scale;

        // 上部中央に配置
        float totalW = numDigits * dw;
        float startX = (m_screenW - totalW) * 0.5f;
        float y = 20.0f;

        // 残り5秒以下で赤くする
        Color tint = (m_timeLeft <= 5.0f) ? Color(1, 0.2f, 0.2f, 1) : Color(1, 1, 1, 1);

        spriteBatch->Begin();
        for (int i = 0; i < numDigits; ++i)
        {
            int digit = buf[i] - L'0';        // 文字を数値0-9に変換
            RECT src;
            src.left = digit * DIGIT_W;
            src.top = 0;
            src.right = src.left + DIGIT_W;
            src.bottom = DIGIT_H;

            Vector2 pos(startX + i * dw, y);
            spriteBatch->Draw(m_numberTexture.Get(), pos, &src, tint,
                0.0f, Vector2::Zero, scale);
        }
        spriteBatch->End();
    }

}