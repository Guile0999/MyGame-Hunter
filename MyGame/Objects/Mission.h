#pragma once
#include "pch.h"
#include <SimpleMath.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <UserResources.h>
#include <MyLib/Utilities/IRaycastTarget.h>

namespace MyLib
{
    // ミッション管理クラス：目標・タイマー・スコア・UI表示を担当
    class Mission
    {
    public:
        Mission();

        void Initialize(UserResources* userResources);                              // 初期化
        void Update(float elapsedTime, bool anyKeyPressed);                         // 更新
        void Render(DirectX::SpriteBatch* spriteBatch, bool bulletCamActive);       // 描画

        void OnHit(IRaycastTarget::HitPart hitPart);   // 命中時の処理（スコア加算）
        void OnMiss();                                 // ミス時の処理（減点）

        void OnBulletFired();   // 発射時に呼ぶ — 「Miss」を先に表示
        void CancelMissText();  // 命中時に呼ぶ — 「Miss!」表示を消す

        // ── ゲッター ──
        bool IsCompleted()  const { return m_result == UserResources::MissionResult::Success; }  // 成功したか
        bool IsFailed()     const { return m_result == UserResources::MissionResult::Failed; }    // 失敗したか
        bool IsChecked()    const { return m_checked; }              // 結果処理済みか
        bool AnyHit()       const { return m_anyHit; }               // 1回でも命中したか
        bool IsBriefingDone() const { return m_briefingDone; }       // ブリーフィング完了か（完了まで撃てない）
        UserResources::MissionResult GetResult()    const { return m_result; }       // 結果を取得
        const std::wstring& GetHitMessage()         const { return m_hitMessage; }   // 命中メッセージを取得

        void MarkChecked() { m_checked = true; }   // 結果処理済みにする
        void OnBossHit();                          // ボス命中時の処理

        // ステージ設定（ステージ番号・目標文・制限時間）
        void SetStage(int stage, const std::wstring& objective, float timeLimit)
        {
            m_stage = stage;
            m_missionObjective = objective;
            m_timeLeft = timeLimit;
        }
        void DrawStageBanner(DirectX::SpriteBatch* spriteBatch);   // ステージバナー描画
        void ForceSuccess();                                       // 強制的に成功にする
        void ForceFail();
       
        // スコア関連
        int GetScore() const { return m_score; }     // 合計スコア
        int GetEarned()  const { return m_earned; }  // 獲得点
        int GetPenalty() const { return m_penalty; } // 減点

        void SetObjective(const std::wstring& text) { m_missionObjective = text; }  // 目標文を更新
        // 目標トラッカーの数値を更新（残り/総数）
        void SetObjectiveCounts(int deerLeft, int deerTotal,
            int rabbitLeft, int rabbitTotal,
            bool hasBoss, bool bossAlive);

    private:
        // イージング関数（滑らかな補間用）
        float Ease(float t) const { return t * t * (3.0f - 2.0f * t); }

        // ミッションパネルの表示フェーズ
        enum class Phase
        {
            Briefing,    // mission_panel.pngを中央表示し入力待ち
            FadeOut,     // mission_panel.pngをフェードアウト
            SlideIn,     // Big.pngを左上の隅へスライド
            Corner       // Big.pngが隅に固定
        };

    private:
        // テクスチャ
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_briefingTexture; // mission_panel.png
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_panelTexture;    // Big.png
        std::unique_ptr<DirectX::SpriteFont> m_font;

        // アニメーション状態
        Phase m_phase;
        float m_phaseTimer;
        float m_fadeAlpha;  // ブリーフィングのフェードアウト用

        // Big.pngパネルの位置・スケール
        DirectX::SimpleMath::Vector2 m_panelPos;
        DirectX::SimpleMath::Vector2 m_panelScale;

        // 画面サイズ
        float m_screenW;
        float m_screenH;

        // Big.png用の定数
        static constexpr float PANEL_W = 606.0f;      // テキスト幅
        static constexpr float PANEL_H = 454.0f;
        static constexpr float SCALE_CENTER = 0.55f;
        static constexpr float SCALE_CORNER = 0.50f;
        static constexpr float TIME_FADEOUT = 0.5f;   // フェードアウト時間
        static constexpr float TIME_SLIDE = 0.8f;     // 隅へのスライド時間

        // mission_panel.png用の定数
        static constexpr float BRIEF_W = 1033.0f;     // 画像サイズ
        static constexpr float BRIEF_H = 785.0f;
        static constexpr float BRIEF_SCALE = 0.65f;   // 画像スケール

        bool m_briefingDone;

        // ミッション状態
        UserResources::MissionResult m_result;
        bool         m_anyHit;   // 1回でも命中したか
        bool         m_checked;  // 結果処理済みか
        std::wstring m_hitMessage;       // 命中メッセージ

        // 目標トラッカーの数値（残り/総数）
        int m_deerLeft , m_deerTotal;
        int m_rabbitLeft, m_rabbitTotal;
        bool m_trackBoss, m_bossAlive;

        // ミス表示の管理
        int  m_lastMissTextIndex;   // 直近のMissテキストのインデックス
        bool m_missTextPending;    // Miss表示が保留中か

        float m_missDisplayTimer;
        bool  m_missTextShowing;
        static constexpr float MISS_DISPLAY_DURATION = 1.5f; // Miss表示の秒数

        //タイマー / ミスシステム 
        float m_timeLeft;
        int   m_missCount;
        static constexpr int   MAX_MISSES = 6;        // 6回ミスで失敗
        static constexpr float START_TIME = 60.0f;    // 開始時間
        static constexpr float HEADSHOT_BONUS = 2.0f; // ヘッドショットの時間ボーナス

        // number.dds 数字シート（0-9、各16x32）
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_numberTexture;
        static constexpr int   DIGIT_W = 16;
        static constexpr int   DIGIT_H = 32;

        // タイマーの減少速度（ミスが増えると速くなる：1.0倍→6ミスで2.0倍）
        float DrainSpeed() const { return 1.0f + (float)m_missCount / (float)MAX_MISSES; }
        void  DrawTimer(DirectX::SpriteBatch* spriteBatch);  // タイマー描画

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_stageTexture;  // Stages.png
        int          m_stage;              // 現在のステージ番号
        std::wstring m_missionObjective; // 目標文

        // ステージバナー用の定数
        static constexpr int STAGE_CELL_W = 512;
        static constexpr int STAGE_CELL_H = 205;   // ~1024/5

        // ステージバナーのアニメーション
        float m_stageBannerTimer;
        float m_stageBannerAlpha;
        static constexpr float STAGE_BANNER_HOLD = 2.0f;   // 2秒間表示
        static constexpr float STAGE_BANNER_FADE = 1.5f;   // 1.5秒かけてフェード

        // スコアUI
        int m_score;

        // スコアの浮きテキスト
        struct FloatingText
        {
            std::wstring text;
            DirectX::SimpleMath::Vector2 pos;
            float timer;
            float duration;
            DirectX::XMVECTORF32 color;
        };
        std::vector<FloatingText> m_floatingTexts;

        // 浮きテキストを出す画面中央の座標
        float m_centerX;
        float m_centerY;

        UserResources* m_userResources;

        // スコア集計
        int m_earned;    // 命中で得た点
        int m_penalty;   // ミスで失った点（正の値で保持）

    };
}