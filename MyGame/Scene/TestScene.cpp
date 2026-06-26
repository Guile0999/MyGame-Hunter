#include <pch.h>
#include <Scene/TestScene.h>
#include <Scene/TitleScene.h>
#include <Objects/FirstPersonPlayer.h>
#include <Objects/FpsGunSetting.h>
#include <Objects/Deer.h>
#include <ReadData.h>
#include <algorithm>
#include <Effects.h>
#include <MyLib/Utilities/BinaryFile.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    // コンストラクタ
    TestScene::TestScene()
        :
        m_bossSpawned(false),                  // ボス出現済みか
        m_showBossWarning(false),              // ボス警告表示中か
        m_bossWarningTimer(0.0f),              // ボス警告タイマー
        m_bulletCamTarget(nullptr),            // 追従中の弾
        m_showScopeText(false),                // スコープテキスト表示
        m_scopeTextTimer(0.0),                 // スコープテキストタイマー
        m_hitMessageTimer(0.0f),               // ヒットメッセージタイマー
        m_cursorHideFrames(5),                 // カーソル非表示フレーム
        m_muzzleFlashTimer(0.0f),              // マズルフラッシュタイマー
        m_muzzleFlashFrame(1),                 // マズルフラッシュフレーム
        m_showMuzzleFlash(false),              // マズルフラッシュ表示中か
        m_killsNeeded(1),                      // 必要キル数
        m_killsGot(0),                         // 現在のキル数
        m_showScorePanel(false),               // スコアパネル表示中か
        m_showBulletHole(false),               // 弾痕表示中か
        m_bulletHolePos(),                     // 弾痕位置
        m_bulletHoleTimer(0.0f),               // 弾痕タイマー
        m_bulletHoleAlpha(1.0f),               // 弾痕アルファ
        m_showDamageEffect(false),             // ダメージ演出中か
        m_damageEffectTimer(0.0f),             // ダメージ演出タイマー
        m_thermalEnabled(false),               // サーマル有効か
        m_boltAnimating(false),                // ボルトアニメ中か
        m_boltTimer(0.0f)                      // ボルトタイマー
    {
    }

    // プールから未使用の弾を探して返す（全て使用中ならnullptr）
    Bullet* TestScene::GetUnusedBullet()
    {
        for (auto& b : m_bulletPool)
            if (b->GetState() == BulletState::Unuse)
                return b.get();
        return nullptr;
    }

    // 初期化
    void TestScene::Initialize()
    {
        auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
        auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

        CreateDeviceDependentResources();
        CreateWindowSizeDependentResources();

        // AudioEngine 初期化
        m_audioEngine = std::make_unique<DirectX::AudioEngine>();
        m_shootSound = std::make_unique<DirectX::SoundEffect>(
            m_audioEngine.get(),
            L"Resources/Sounds/Sniper.wav"
        );

        // 環境音を読み込む（森の雰囲気）
        m_windSound = std::make_unique<DirectX::SoundEffect>(
            m_audioEngine.get(), L"Resources/Sounds/Wind_Forest_loop.wav");
        m_birdsSound = std::make_unique<DirectX::SoundEffect>(
            m_audioEngine.get(), L"Resources/Sounds/Birds_loop.wav");
        m_cicadasSound = std::make_unique<DirectX::SoundEffect>(
            m_audioEngine.get(), L"Resources/Sounds/Cicadas_loop.wav");

        // ループ再生用インスタンスを作成
        m_windInstance = m_windSound->CreateInstance();
        m_birdsInstance = m_birdsSound->CreateInstance();
        m_cicadasInstance = m_cicadasSound->CreateInstance();

        // 音量を調整（環境音は控えめに）
        m_windInstance->SetVolume(0.4f);
        m_birdsInstance->SetVolume(0.5f);
        m_cicadasInstance->SetVolume(0.3f);

        // ループ再生を開始（trueでループ）
        m_windInstance->Play(true);
        m_birdsInstance->Play(true);
        m_cicadasInstance->Play(true);

        // --- ステージ設定 ---
        m_levels.SetStage(GetUserResources()->currentStage);
        m_killsNeeded = m_levels.GetKillsNeeded();
        m_killsGot = 0;

        // レイキャスターはターゲット追加より先に作る
        m_rayCaster = std::make_shared<RaycastSystem>();

        // 鹿をスポーン（経路はlevels.jsonから取得）
        const auto& deerPaths = m_levels.GetDeerPaths();
        for (int i = 0; i < m_levels.GetDeerCount(); i++)
        {
            Vector3 pos(-22.0f + i * 10.0f, -1.0f, 31.0f);
            auto deer = std::make_unique<Deer>(pos, 1.0f);
            deer->Initialize(context);
            deer->LoadModel(device);
            deer->ShowCollisionBox(false);
            if (!deerPaths.empty())
                deer->SetWaypoints(deerPaths[i % deerPaths.size()]);
            deer->SetMovementVariation(0, 1.0f + 0.1f * i);
            m_rayCaster->AddTarget(deer.get());
            m_deers.push_back(std::move(deer));
        }

        // ウサギをスポーン（経路はlevels.jsonから取得）
        const auto& rabbitPaths = m_levels.GetRabbitPaths();
        for (int i = 0; i < m_levels.GetRabbitCount(); i++)
        {
            Vector3 pos(-15.0f + i * 8.0f, -1.0f, 28.0f);
            auto rabbit = std::make_unique<Rabbit>(pos, 0.5f);
            rabbit->Initialize(context);
            rabbit->LoadModel(device);
            rabbit->ShowCollisionBox(false);
            if (!rabbitPaths.empty())
                rabbit->SetWaypoints(rabbitPaths[i % rabbitPaths.size()]);
            rabbit->SetMovementVariation(0, 1.0f + 0.1f * i);
            m_rayCaster->AddTarget(rabbit.get());
            m_rabbits.push_back(std::move(rabbit));
        }

        // プレイヤー生成
        m_player = std::make_unique<FirstPersonPlayer>(PlayerSettings{});
        m_player->InitializeCamera(45.0f, 0.1f, 1000.0f);
        m_player->SetPosition(Vector3(0, 0.0f, 0));

        // Gunの生成
        m_fpsGun = std::make_unique<FpsGun>();
        m_fpsGun->Initialize(FpsGunSetting{});

        // スコープ初期化
        m_scope = std::make_unique<Scope>();
        m_scope->Initialize(GetUserResources());

        // 床
        m_floor = std::make_unique<FloorPrimitive>(device, 150.0f);
        m_floor->SetPosition(Vector3(0, -1.0f, 0));

        // UI
        m_debugFont = std::make_unique<DebugFont>(device, context, L"Resources/Font/SegoeUI_18.spritefont");
        m_spriteBatch = std::make_unique<SpriteBatch>(context);

        // マガジンUI（音声エンジンを先に渡す）
        m_mag = std::make_unique<Magazine>();
        m_mag->SetAudioEngine(m_audioEngine.get());
        auto rect = GetUserResources()->GetDeviceResources()->GetOutputSize();
        m_mag->Initialize(GetUserResources()->GetDeviceResources(), rect.right, rect.bottom);

        // ステージ初期化
        m_stage = std::make_unique<Stage>();
        auto fx = std::make_unique<EffectFactory>(device);
        fx->SetDirectory(L"Resources/Models");
        m_stage->Initialize(device, context, *fx);

        // 弾プールを事前生成（発射時に再利用する）
        m_bulletPool.clear();
        for (int i = 0; i < BULLET_POOL_SIZE; i++)
            m_bulletPool.push_back(std::make_unique<Bullet>());   // 全てUnuse状態

        CameraManager::CreateInstance()->Init(m_player.get());

        // ミッション初期化
        m_mission = std::make_unique<Mission>();
        m_mission->Initialize(GetUserResources());
        m_mission->SetStage(m_levels.GetStage(), m_levels.GetMissionText(), m_levels.GetTimeLimit());

        while (ShowCursor(FALSE) >= 0) {}
        DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_RELATIVE);
    }

    // 更新：入力処理と各サブ更新（発射・弾・ボス・カメラ等）を順に呼ぶ
    void TestScene::Update(float elapsedTime)
    {
       
        // 開始数フレームはカーソルを隠す
        HideCursorFirstFrames();

        // 入力状態の取得
        auto kb = Keyboard::Get().GetState();
        auto mouse = Mouse::Get().GetState();
        m_mouseTracker.Update(mouse);

        // スコアパネル中なら停止
        if (HandleScorePanel())
            return;

        // バレットカメラが有効か（弾追従／停止中）
        bool bulletCamActive =
            CameraManager::GetInstance()->GetState() == CamState::Bullet ||
            CameraManager::GetInstance()->GetState() == CamState::Freeze;

        // 何か入力されたか（ブリーフィングのスキップ判定用）
        bool anyKeyPressed =
            m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED ||
            m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED ||
            kb.Space || kb.Enter;

        // ミッション更新
        m_mission->Update(elapsedTime, anyKeyPressed);

        // サーマルビジョン切り替え（スペースキー）
        static bool spacePressedLastFrame = false;
        if (kb.Space && !spacePressedLastFrame && m_mission->IsBriefingDone())
            m_thermalEnabled = !m_thermalEnabled;
        spacePressedLastFrame = kb.Space;

        // プレイヤーと銃の更新
        m_player->Update(elapsedTime, kb, *GetUserResources()->GetKeyboardStateTracker(), mouse);
        m_fpsGun->Update(*m_player);

        // 鹿の更新 + アニメーション
        for (auto& deer : m_deers)
        {
            deer->Update(elapsedTime);
            deer->UpdateAnimation(elapsedTime);
        }
          // ミッションパネルの目標トラッカーを更新
        {
            int deerTotal = (int)m_deers.size();
            int deerLeft = 0;
            for (auto& d : m_deers)
                if (d->IsAlive()) deerLeft++;

            int rabbitTotal = (int)m_rabbits.size();
            int rabbitLeft = 0;
            for (auto& r : m_rabbits)
                if (r->IsAlive()) rabbitLeft++;

            bool hasBoss = m_levels.HasBoss() && m_bossSpawned;
            bool bossAlive = (m_boss && m_boss->IsAlive());

            m_mission->SetObjectiveCounts(deerLeft, deerTotal, rabbitLeft, rabbitTotal, hasBoss, bossAlive);
        }

        // ウサギの更新 + アニメーション
        for (auto& rabbit : m_rabbits)
        {
            rabbit->Update(elapsedTime);
            rabbit->UpdateAnimation(elapsedTime);
        }

        // ボスの出現・更新・ダメージ演出
        UpdateBoss(elapsedTime);

        // 発射処理（バレットカメラ中でなく、ブリーフィング後、左クリック時）
        if (!bulletCamActive &&
            m_mission->IsBriefingDone() &&
            m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
            UpdateFiring();

        // 弾の更新＋命中判定
        UpdateBullets(elapsedTime);
        UpdateBulletCamera(elapsedTime);

        // 完了・失敗・シーン遷移チェック
        UpdateCompletion();
        // ミッション終了でスコアパネルに入ったらここで抜ける
        if (m_showScorePanel)
            return;

        // ビュー／射影行列を決定
        UpdateCameraMatrices();

        UpdateScope(elapsedTime, bulletCamActive);
        // ステージ更新
        if (m_stage)
            m_stage->Update(elapsedTime);

        // UI・オーディオ更新
        if (m_mag)         
            m_mag->Update(elapsedTime);

        if (m_audioEngine) 
            m_audioEngine->Update();

        // マズルフラッシュ・弾痕などのタイマー
        UpdateTimers(elapsedTime);
    }

    // 発射処理
    void TestScene::UpdateFiring()
    {
       
            if (m_mag && m_mag->GetAmmoCount() > 0 && m_fpsGun && m_bulletModel)
            {
                Bullet* bullet = GetUnusedBullet();
                if (bullet)
                {
                    // FpsGunが発射情報入りのBulletを返すので、レイ情報をプール弾へ転写する
                    Bullet shot = m_fpsGun->Shoot(*m_player, m_bulletModel, *m_rayCaster);
                    MyLib::Ray r = shot.GetRay();
                    bullet->Reset(r.origin, r.direction, 50.0f, m_bulletModel);

                    // 最初のキル/最後のキル/ボス戦の弾はバレットカメラで追う
                    bool couldBeFirstKill = (m_killsGot == 0);
                    bool couldBeLastKill = (!m_levels.HasBoss() && m_killsGot == m_killsNeeded - 1);
                    bool bossLastShot = (m_boss && m_boss->IsAlive());

                    if (couldBeFirstKill || couldBeLastKill || bossLastShot)
                    {
                        m_bulletCamTarget = bullet;
                        CameraManager::GetInstance()->OnFire(bullet);
                    }

                    m_showMuzzleFlash = true;
                    m_muzzleFlashTimer = 0.0f;
                    m_muzzleFlashFrame = rand() % 2;

                    m_mag->Shoot();

                    if (m_shootSound)
                    {
                        auto instance = m_shootSound->CreateInstance();
                        instance->Play();
                        m_activeSoundInstances.push_back(std::move(instance));
                    }
                }
            }
    }

    // 弾の更新＋命中判定
    void TestScene::UpdateBullets(float elapsedTime)
    {
        // 弾の更新（プールを走査して飛行中のみ処理）
        for (auto& bulletPtr : m_bulletPool)
        {
            Bullet* bullet = bulletPtr.get();
            if (bullet->GetState() != BulletState::Flying)
                continue;

            float thisTime = (bullet == m_bulletCamTarget)
                ? elapsedTime * 0.03f
                : elapsedTime;

            bullet->Update(thisTime, *m_rayCaster, 900.0f);

            if (bullet->DidHit())
            {
                auto hitPart = bullet->GetHitPart();
                auto hitTarget = bullet->GetHitTarget();

                // DEER HIT — どの鹿か探す
                for (auto& deer : m_deers)
                {
                    if (hitTarget == deer.get())
                    {
                        if (!deer->IsAlive())
                        {
                            m_mission->CancelMissText();
                            m_mission->OnHit(hitPart);
                            m_killsGot++;
                            m_showBulletHole = true;
                            m_bulletHoleTimer = 0.0f;
                            m_bulletHoleAlpha = 1.0f;
                            m_bulletHolePos = (hitPart == IRaycastTarget::HitPart::HEAD)
                                ? deer->GetHeadCenter()
                                : deer->GetBodyCenter();
                        }
                        break;
                    }
                }

                // RABBIT HIT
                for (auto& rabbit : m_rabbits)
                {
                    if (hitTarget == rabbit.get())
                    {
                        if (!rabbit->IsAlive())
                        {
                            m_mission->CancelMissText();
                            m_mission->OnHit(hitPart);
                            m_killsGot++;
                            m_showBulletHole = true;
                            m_bulletHoleTimer = 0.0f;
                            m_bulletHoleAlpha = 1.0f;
                            m_bulletHolePos = rabbit->GetBodyCenter();
                        }
                        break;
                    }
                }

                // BOSS HIT
                if (m_boss && hitTarget == m_boss.get())
                {
                    m_mission->CancelMissText();
                    m_mission->OnBossHit();
                    m_showBulletHole = true;
                    m_bulletHoleTimer = 0.0f;
                    m_bulletHoleAlpha = 1.0f;
                    m_bulletHolePos = m_boss->GetLungCenter();
                }

                // バレットカメラ
                if (bullet == m_bulletCamTarget)
                {
                    if (hitPart == IRaycastTarget::HitPart::HEAD ||
                        hitPart == IRaycastTarget::HitPart::BODY ||
                        hitPart == IRaycastTarget::HitPart::LUNG)
                        CameraManager::GetInstance()->OnBulletHit();
                    else
                        CameraManager::GetInstance()->OnBulletLost();
                    m_bulletCamTarget = nullptr;
                }

                bullet->SetState(BulletState::Unuse);   // プールへ返却
            }
            else if (bullet->JustExpired())
            {
                if (bullet == m_bulletCamTarget)
                {
                    CameraManager::GetInstance()->OnBulletLost();
                    m_bulletCamTarget = nullptr;
                }

                // MISS — 登録 + 全動物を怯えさせる
                m_mission->OnMiss();
                for (auto& deer : m_deers)
                    deer->Spook();
                for (auto& rabbit : m_rabbits)
                    rabbit->rabbitSpook();

                bullet->SetState(BulletState::Unuse);   // プールへ返却
            }
        }

    }

    // ボス出現・更新・ダメージ
    void TestScene::UpdateBoss(float elapsedTime)
    {
        
        // Stage 3: 一定数キルでボス出現 
        if (m_levels.HasBoss() && !m_bossSpawned &&
            m_killsGot >= m_levels.GetBossTriggerKills())
        {
            auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();
            auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();

            m_boss = std::make_unique<Boss>(Vector3(0.0f, -1.0f, 30.0f), 1.5f);
            m_boss->Initialize(context);
            m_boss->LoadModel(device);
            m_boss->ShowCollisionBox(false);
            m_rayCaster->AddTarget(m_boss.get());

            m_bossSpawned = true;
            m_showBossWarning = true;
            m_bossWarningTimer = 0.0f;
            m_mission->SetObjective(L"Defeat the boss!");
        }

        // ボス警告バナーのタイマー
        if (m_showBossWarning)
        {
            m_bossWarningTimer += elapsedTime;
            if (m_bossWarningTimer >= BOSS_WARNING_DURATION)
                m_showBossWarning = false;
        }

        // ボスの更新
        if (m_boss)
        {
            m_boss->SetPlayerPosition(m_player->GetPosition());
            m_boss->Update(elapsedTime);
            m_boss->UpdateAnimation(elapsedTime);

            // ボスがプレイヤーに攻撃した → ダメージ演出開始
            if (m_boss->HasHitPlayer() && !m_showDamageEffect)
            {
                m_showDamageEffect = true;
                m_damageEffectTimer = 0.0f;
                m_boss->ResetHitPlayer();
            }
        }

        // ダメージ演出のタイマー → 一定時間後にミッション失敗
        if (m_showDamageEffect)
        {
            m_damageEffectTimer += elapsedTime;
            if (m_damageEffectTimer >= DAMAGE_EFFECT_DURATION)
            {
                m_showDamageEffect = false;
                m_mission->ForceFail();
            }
        }

    }

    // 完了・シーン遷移チェック
    void TestScene::UpdateCompletion()
    {

        // ミッション失敗チェック — 弾切れ・命中なし
        if (m_mag &&
            m_mag->GetAmmoCount() == 0 &&
            !m_mission->AnyHit() &&
            m_mission->GetResult() == UserResources::MissionResult::None)
        {
            bool lastBulletStillFlying =
                (m_bulletCamTarget != nullptr && m_bulletCamTarget->IsActive());

            if (!lastBulletStillFlying)
                m_mission->OnMiss();
        }

       
        // 完了チェック
        if (m_mission->GetResult() == UserResources::MissionResult::None)
        {
            if (m_levels.HasBoss())
            {
                // Stage 3: ボスが死んだら勝ち
                if (m_bossSpawned && m_boss && !m_boss->IsAlive())
                    m_mission->ForceSuccess();
            }
            else
            {
                // Stage 1 & 2: 必要数の動物を倒したら勝ち
                if (m_killsGot >= m_killsNeeded)
                    m_mission->ForceSuccess();
            }
        }

        // シーン遷移 — ミッション終了
        if (m_mission->GetResult() != UserResources::MissionResult::None &&
            !m_mission->IsChecked())
        {
            bool allDeadAnimDone = true;
            for (auto& deer : m_deers)
                if (!deer->IsAlive() && !deer->IsDeadAnimDone())
                    allDeadAnimDone = false;
            for (auto& rabbit : m_rabbits)
                if (!rabbit->IsAlive() && !rabbit->IsDeadAnimDone())
                    allDeadAnimDone = false;
            if (m_boss && !m_boss->IsAlive() && !m_boss->IsDeadAnimDone())
                allDeadAnimDone = false;

            if (allDeadAnimDone)
            {
                m_mission->MarkChecked();
                GetUserResources()->finalScore = m_mission->GetScore();
                GetUserResources()->missionResult = m_mission->GetResult();
                CameraManager::GetInstance()->OnBulletLost();
                m_showScorePanel = true;
                return;
            }
        }
    }

    // ビュー／射影行列の決定
    void TestScene::UpdateCameraMatrices()
    {
        // ビュー行列
        if (CameraManager::GetInstance()->GetState() == CamState::Bullet ||
            CameraManager::GetInstance()->GetState() == CamState::Freeze)
        {
            m_view = CameraManager::GetInstance()->GetActiveView();
        }
        else
        {
            m_view = m_player->GetViewMatrix();
        }

        // プロジェクション行列
        RECT  rect = GetUserResources()->GetDeviceResources()->GetOutputSize();
        float aspect = float(rect.right) / float(rect.bottom);

        if (CameraManager::GetInstance()->GetState() == CamState::Bullet ||
            CameraManager::GetInstance()->GetState() == CamState::Freeze)
        {
            m_proj = CameraManager::GetInstance()->GetActiveProjection(aspect);
        }
        else if (m_scope && m_scope->IsActive())
        {
            float scopedFov = 45.0f / m_scope->GetZoomFactor();
            m_proj = Matrix::CreatePerspectiveFieldOfView(
                XMConvertToRadians(scopedFov), aspect, 0.1f, 1000.0f);
        }
        else
        {
            m_proj = m_player->GetProjectionMatrix(aspect);
        }

    }

    // マズルフラッシュ・弾痕などのタイマ
    void TestScene::UpdateTimers(float elapsedTime)
    {
        // マズルフラッシュタイマー
        if (m_showMuzzleFlash)
        {
            m_muzzleFlashTimer += elapsedTime;
            if (m_muzzleFlashTimer >= MUZZLE_FLASH_DURATION)
                m_showMuzzleFlash = false;
        }

        // 弾痕フェードタイマー
        if (m_showBulletHole)
        {
            m_bulletHoleTimer += elapsedTime;
            m_bulletHoleAlpha = 1.0f - (m_bulletHoleTimer / BULLET_HOLE_DURATION);
            if (m_bulletHoleTimer >= BULLET_HOLE_DURATION)
            {
                m_showBulletHole = false;
                m_bulletHoleTimer = 0.0f;
            }
        }
    }

    // スコープ更新・切替
    void TestScene::UpdateScope(float elapsedTime, bool bulletCamActive)
    {

        // スコープ更新（息止めはLeftControl）
        if (m_scope)
        {
            bool holdingBreath = Keyboard::Get().GetState().LeftControl;
            m_scope->Update(elapsedTime, holdingBreath);
        }

        // スコープ状態の切り替え（右クリック）
        if (!bulletCamActive &&
            m_mission->IsBriefingDone() &&
            m_mouseTracker.rightButton == Mouse::ButtonStateTracker::PRESSED)
        {
            m_scope->CycleState();
            m_showScopeText = true;
            m_scopeTextTimer = 0.0;
            m_scopeDebugText = m_scope->IsActive() ? L"Scope ON" : L"Scope OFF";
        }

        // スコープ切替テキストの表示タイマー
        if (m_showScopeText)
        {
            m_scopeTextTimer += elapsedTime;
            if (m_scopeTextTimer > 1.5)
                m_showScopeText = false;
        }

    }

    // バレットカメラ更新
    void TestScene::UpdateBulletCamera(float elapsedTime)
    {
        // バレットカメラ更新（追従中の弾を渡す）
        Bullet* activeCamBullet = nullptr;
        if (m_bulletCamTarget && m_bulletCamTarget->IsActive())
            activeCamBullet = m_bulletCamTarget;

        float camTime = activeCamBullet ? elapsedTime * 0.05f : elapsedTime;
        CameraManager::GetInstance()->Update(camTime, activeCamBullet);
    }

    // 開始数フレームのカーソル非表示
    void TestScene::HideCursorFirstFrames()
    {
        // 最初の数フレームはカーソルを強制的に隠す
        if (m_cursorHideFrames > 0)
        {
            while (ShowCursor(FALSE) >= 0) {}
            m_cursorHideFrames--;
        }
    }

    // スコアパネル中ならtrueを返す（更新を止める）
    bool TestScene::HandleScorePanel()
    {

        // スコアパネル表示中 — ゲームを止めてクリック待ち
        if (!m_showScorePanel)
            return false;

        if (m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
        {
            CameraManager::GetInstance()->OnBulletLost();
            ChangeScene<ResultScene>();
        }
        return true;
    }

    // 3Dシーン描画
    void TestScene::DrawSceneObjects()
    {
        bool bulletCamActive =
            CameraManager::GetInstance()->GetState() == CamState::Bullet ||
            CameraManager::GetInstance()->GetState() == CamState::Freeze;

        auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();
        auto states = GetUserResources()->GetCommonStates();

        // 鹿の描画
        for (auto& deer : m_deers)
        {
            // 死亡してアニメが終わった鹿は描画しない（消す）
            if (!deer->IsAlive() && deer->IsDeadAnimDone())
                continue;

            deer->DrawModel(context, *states, m_view, m_proj);
            if (deer->IsAlive())
                deer->Render(m_view, m_proj);
        }
        

        // ウサギの描画
        for (auto& rabbit : m_rabbits)
        {
            // 死亡してアニメが終わったウサギは描画しない（消す）
            if (!rabbit->IsAlive() && rabbit->IsDeadAnimDone())
                continue;

            rabbit->DrawModel(context, *states, m_view, m_proj);
            if (rabbit->IsAlive())
                rabbit->Render(m_view, m_proj);
        }

        // ボスの描画
        if (m_boss)
        {
            if (!(!m_boss->IsAlive() && m_boss->IsDeadAnimDone()))
            {
                m_boss->DrawModel(context, *states, m_view, m_proj);
                if (m_boss->IsAlive())
                    m_boss->Render(m_view, m_proj);
            }
        }

        // 弾痕デカール
        if (m_showBulletHole && m_bulletHoleTexture)
        {
            Vector3 clipPos = Vector3::Transform(m_bulletHolePos, m_view * m_proj);
            if (clipPos.z > 0.0f)
            {
                RECT outputRect = GetUserResources()->GetDeviceResources()->GetOutputSize();
                Vector2 screenPos;
                screenPos.x = (clipPos.x * 0.5f + 0.5f) * outputRect.right;
                screenPos.y = (-clipPos.y * 0.5f + 0.5f) * outputRect.bottom;

                m_spriteBatch->Begin(SpriteSortMode_Deferred,
                    GetUserResources()->GetCommonStates()->NonPremultiplied());
                m_spriteBatch->Draw(
                    m_bulletHoleTexture.Get(),
                    screenPos, nullptr,
                    Color(1.0f, 1.0f, 1.0f, m_bulletHoleAlpha),
                    0.0f, Vector2(450.0f, 450.0f), 0.05f
                );
                m_spriteBatch->End();
            }
        }

        // 弾の描画（プールから飛行中のみ）
        for (auto& bulletPtr : m_bulletPool)
        {
            Bullet* bullet = bulletPtr.get();
            if (bullet->GetState() != BulletState::Flying)
                continue;
            bullet->Render(context, m_view, m_proj, *states);
            if (bulletCamActive)
                bullet->RenderTrail(context, m_view, m_proj);
        }

        if (m_floor)
            m_floor->Render(context, m_view, m_proj);

        if (m_stage)
            m_stage->Render(context, m_view, m_proj, *states);

        if (!bulletCamActive && m_fpsGunModel && m_fpsGun && (!m_scope || !m_scope->IsActive()))
            m_fpsGunModel->Draw(context, *states, m_fpsGun->GetWorldMatrix(), m_view, m_proj);
    }

    // 描画
    void TestScene::Render()
    {
        bool bulletCamActive =
            CameraManager::GetInstance()->GetState() == CamState::Bullet ||
            CameraManager::GetInstance()->GetState() == CamState::Freeze;

        DrawSceneObjects();

        auto deviceResources = GetUserResources()->GetDeviceResources();
        auto states = GetUserResources()->GetCommonStates();
        RECT rect = deviceResources->GetOutputSize();
        Vector2 screenCenter(rect.right * 0.5f, rect.bottom * 0.5f);

        if (!bulletCamActive)
            m_mission->Render(m_spriteBatch.get(), bulletCamActive);

        m_spriteBatch->Begin();

        if (!bulletCamActive)
        {
            if (m_crosshairTexture && (!m_scope || !m_scope->IsActive()))
            {
                m_spriteBatch->Draw(
                    m_crosshairTexture.Get(), screenCenter, nullptr,
                    Colors::White, 0.0f, m_crosshairOrigin, 0.1f
                );
            }

            if (m_showScopeText)
                m_debugFont->AddString(m_scopeDebugText.c_str(), { 20, 20 }, Colors::Cyan);

            if (m_thermalEnabled)
                m_debugFont->AddString(L"Thermal ON", { 20, 150 }, Colors::Red);
        }

        if (!m_mission->GetHitMessage().empty())
            m_debugFont->AddString(m_mission->GetHitMessage().c_str(), { 20, 100 }, Colors::Orange);

        m_debugFont->Render(states);
        m_spriteBatch->End();

        // マズルフラッシュ（2枚重ねの大きめ表現）
        if (m_showMuzzleFlash && !bulletCamActive && m_muzzleFlashTexture &&
            !m_showScorePanel &&
            m_fpsGun && (!m_scope || !m_scope->IsActive()))
        {
            RECT outputRect = GetUserResources()->GetDeviceResources()->GetOutputSize();
            Vector2 screenPos(float(outputRect.right) * 0.55f, float(outputRect.bottom) * 0.55f);

            m_spriteBatch->Begin(SpriteSortMode_Deferred,
                GetUserResources()->GetCommonStates()->Additive());

            // 1枚目：白く大きく
            m_spriteBatch->Draw(
                m_muzzleFlashTexture.Get(), screenPos, nullptr,
                DirectX::Colors::White,
                static_cast<float>(rand() % 360) * DirectX::XM_PI / 180.0f,
                Vector2(256.0f, 128.0f), 1.2f
            );
            // 2枚目：暖色で少し小さく
            m_spriteBatch->Draw(
                m_muzzleFlashTexture.Get(), screenPos, nullptr,
                Color(1.0f, 0.9f, 0.6f, 0.8f),
                static_cast<float>(rand() % 360) * DirectX::XM_PI / 180.0f,
                Vector2(256.0f, 128.0f), 0.8f
            );
            m_spriteBatch->End();
        }

        if (m_mag && !bulletCamActive)
            m_mag->Render();

        // スコープ + サーマル
        if (!bulletCamActive && m_scope && m_scope->IsActive())
        {
            m_scope->Render(Vector2((float)rect.right, (float)rect.bottom));

            if (m_thermalEnabled && m_scopeOverlayTexture)
            {
                m_spriteBatch->Begin();
                m_spriteBatch->Draw(
                    m_scopeOverlayTexture.Get(), screenCenter, nullptr,
                    Color(0.05f, 0.1f, 0.35f, 0.85f),
                    0.0f, Vector2(0.5f, 0.5f),
                    Vector2((float)rect.right, (float)rect.bottom)
                );
                m_spriteBatch->End();
            }

            // 動物のサーマルグロー
            if (m_thermalEnabled && m_thermalGlowTexture)
            {
                RECT r = GetUserResources()->GetDeviceResources()->GetOutputSize();

                struct GlowPoint { Vector3 worldPos; float size; Color color; };

                auto drawGlow = [&](const std::vector<GlowPoint>& heatPoints)
                    {
                        for (const auto& point : heatPoints)
                        {
                            Vector3 clipPos = Vector3::Transform(point.worldPos, m_view * m_proj);
                            if (clipPos.z <= 0.0f || !std::isfinite(clipPos.x) || !std::isfinite(clipPos.y))
                                continue;

                            Vector2 screenPos;
                            screenPos.x = (clipPos.x * 0.5f + 0.5f) * r.right;
                            screenPos.y = (-clipPos.y * 0.5f + 0.5f) * r.bottom;

                            Vector3 camPos = m_view.Invert().Translation();
                            float distance = Vector3::Distance(camPos, point.worldPos);
                            float distScale = std::max(0.01f, point.size * (20.0f / distance));

                            m_spriteBatch->Begin(SpriteSortMode_Deferred,
                                GetUserResources()->GetCommonStates()->Additive());
                            m_spriteBatch->Draw(
                                m_thermalGlowTexture.Get(), screenPos, nullptr,
                                point.color, 0.0f, Vector2(0.2f, 0.2f), distScale
                            );
                            m_spriteBatch->End();
                        }
                    };

                // 鹿
                for (auto& deer : m_deers)
                {
                    if (!deer->IsAlive()) continue;
                    drawGlow({
                        { deer->GetBodyCenter(), 0.06f,  Color(1.0f, 0.6f, 0.1f, 1.0f) },
                        { deer->GetHeadCenter(), 0.025f, Color(1.0f, 1.0f, 0.6f, 1.0f) }
                        });
                }
                // ウサギ
                for (auto& rabbit : m_rabbits)
                {
                    if (!rabbit->IsAlive()) continue;
                    drawGlow({
                        { rabbit->GetBodyCenter(), 0.04f, Color(1.0f, 0.6f, 0.1f, 1.0f) },
                        { rabbit->GetHeadCenter(), 0.02f, Color(1.0f, 1.0f, 0.6f, 1.0f) }
                        });
                }
                // ボス
                if (m_boss && m_boss->IsAlive())
                {
                    drawGlow({
                        { m_boss->GetBodyCenter(), 0.08f,  Color(1.0f, 0.5f, 0.1f, 1.0f) },
                        { m_boss->GetLungCenter(), 0.05f,  Color(1.0f, 0.8f, 0.3f, 1.0f) },
                        { m_boss->GetHeadCenter(), 0.035f, Color(1.0f, 1.0f, 0.6f, 1.0f) }
                        });
                }
            }
        }

        // ボス警告バナー（中央・点滅）
        if (m_showBossWarning && m_bossWarningTexture && !bulletCamActive)
        {
            RECT r = GetUserResources()->GetDeviceResources()->GetOutputSize();
            float sw = float(r.right), sh = float(r.bottom);

            bool visible = (fmodf(m_bossWarningTimer, 0.3f) < 0.15f);
            if (visible)
            {
                float scale = sw / 1536.0f;
                float bannerH = 1024.0f * scale;
                Vector2 pos(0.0f, (sh - bannerH) * 0.5f);

                m_spriteBatch->Begin(SpriteSortMode_Deferred,
                    GetUserResources()->GetCommonStates()->NonPremultiplied());
                m_spriteBatch->Draw(m_bossWarningTexture.Get(), pos, nullptr,
                    Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));
                m_spriteBatch->End();
            }
        }

        // ダメージ演出（ボス攻撃時の赤いビネット）
        if (m_showDamageEffect && m_damageTexture)
        {
            RECT r = GetUserResources()->GetDeviceResources()->GetOutputSize();
            float sw = float(r.right), sh = float(r.bottom);

            float alpha = std::min(1.0f, m_damageEffectTimer * 3.0f);

            m_spriteBatch->Begin(SpriteSortMode_Deferred,
                GetUserResources()->GetCommonStates()->NonPremultiplied());
            m_spriteBatch->Draw(m_damageTexture.Get(),
                RECT{ 0, 0, (LONG)sw, (LONG)sh }, nullptr,
                Color(1.0f, 1.0f, 1.0f, alpha));
            m_spriteBatch->End();
        }

        // スコアパネル（停止した3Dシーンの上に表示）
        if (m_showScorePanel && m_scorePanelTexture)
        {
            RECT r = GetUserResources()->GetDeviceResources()->GetOutputSize();
            float sw = float(r.right), sh = float(r.bottom);

            m_spriteBatch->Begin();

            float panelW = 606.0f, panelH = 454.0f, scale = 1.0f;
            Vector2 panelPos((sw - panelW * scale) * 0.5f, (sh - panelH * scale) * 0.5f);
            m_spriteBatch->Draw(m_scorePanelTexture.Get(), panelPos, nullptr,
                Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));

            if (m_scoreFont)
            {
                bool success = (m_mission->GetResult() == UserResources::MissionResult::Success);
                const wchar_t* title = success ? L"OBJECTIVE CLEAR" : L"MISSION FAILED";
                XMVECTOR tsz = m_scoreFont->MeasureString(title);
                float tw = XMVectorGetX(tsz) * 1.5f;
                m_scoreFont->DrawString(m_spriteBatch.get(), title,
                    Vector2(sw * 0.5f - tw * 0.5f, sh * 0.32f),
                    Colors::Gold, 0.0f, Vector2::Zero, 1.5f);

                // Earned
                wchar_t earnedBuf[64];
                swprintf_s(earnedBuf, L"Earned:  +%d", m_mission->GetEarned());
                m_scoreFont->DrawString(m_spriteBatch.get(), earnedBuf,
                    Vector2(sw * 0.5f - 120.0f, sh * 0.44f),
                    Colors::LightGreen, 0.0f, Vector2::Zero, 1.0f);

                // Penalty
                wchar_t penaltyBuf[64];
                swprintf_s(penaltyBuf, L"Penalty: -%d", m_mission->GetPenalty());
                m_scoreFont->DrawString(m_spriteBatch.get(), penaltyBuf,
                    Vector2(sw * 0.5f - 120.0f, sh * 0.50f),
                    Colors::Red, 0.0f, Vector2::Zero, 1.0f);

                // Total
                wchar_t cashBuf[64];
                swprintf_s(cashBuf, L"TOTAL:  $ %d", m_mission->GetScore());
                XMVECTOR csz = m_scoreFont->MeasureString(cashBuf);
                float cw = XMVectorGetX(csz) * 1.3f;
                m_scoreFont->DrawString(m_spriteBatch.get(), cashBuf,
                    Vector2(sw * 0.5f - cw * 0.5f, sh * 0.58f),
                    Colors::White, 0.0f, Vector2::Zero, 1.3f);

                const wchar_t* hint = L"Left Click to continue";
                XMVECTOR hsz = m_scoreFont->MeasureString(hint);
                float hw = XMVectorGetX(hsz) * 0.8f;
                m_scoreFont->DrawString(m_spriteBatch.get(), hint,
                    Vector2(sw * 0.5f - hw * 0.5f, sh * 0.66f),
                    Colors::LightGray, 0.0f, Vector2::Zero, 0.8f);
            }

            m_spriteBatch->End();
        }
    }

    // デバイス依存リソース作成
    void TestScene::CreateDeviceDependentResources()
    {
        auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
        if (!device) throw std::runtime_error("D3D device not valid.");

        auto fx = std::make_unique<EffectFactory>(device);
        fx->SetDirectory(L"Resources/Models");

        m_fpsGunModel = Model::CreateFromSDKMESH(device, L"Resources/Models/Player/rifleTest.sdkmesh", *fx);
        // 銃モデルにライティングを有効化（標準の3点ライト）
        m_fpsGunModel->UpdateEffects([](IEffect* effect)
            {
                // ライト対応エフェクトならライティングをオン
                auto lights = dynamic_cast<IEffectLights*>(effect);
                if (lights)
                {
                    lights->SetLightingEnabled(true);
                    lights->EnableDefaultLighting();
                    // 例：1つ目のライトの向きと色を調整
                    lights->SetLightDirection(0, Vector3(0.8f, -1.0f, 0.8f));
                    lights->SetLightDiffuseColor(0, Colors::White);
                    lights->SetAmbientLightColor(Vector3(1.0f, 1.0f, 1.0f));  // 全体の最低明るさ
                }
                // BasicEffectならピクセル単位ライティングにする（より滑らか）
                auto basic = dynamic_cast<BasicEffect*>(effect);
                if (basic)
                    basic->SetPerPixelLighting(true);
            });
        m_bulletModel = Model::CreateFromSDKMESH(device, L"Resources/Models/bullet.sdkmesh", *fx);

        // テクスチャ読み込み
        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/blood_splat.png",
            nullptr, m_bulletHoleTexture.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/crossHair.png",
            nullptr, m_crosshairTexture.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/muzzleflash.png",
            nullptr, m_muzzleFlashTexture.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/Thermal.png",
            nullptr, m_thermalGlowTexture.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/Big.png",
            nullptr, m_scorePanelTexture.ReleaseAndGetAddressOf()));

        // ボス警告バナー
        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/UI.png",
            nullptr, m_bossWarningTexture.ReleaseAndGetAddressOf()));

        // ダメージ演出（赤いビネット）
        DX::ThrowIfFailed(CreateWICTextureFromFile(device,
            L"Resources/Textures/Effect_3.png",
            nullptr, m_damageTexture.ReleaseAndGetAddressOf()));

        m_scoreFont = std::make_unique<SpriteFont>(device,
            L"Resources/Font/SegoeUI_18.spritefont");

        // シェーダー読み込み
        auto vsData = MyLib::BinaryFile::LoadFile(L"Resources/Shaders/TharmalScope_VS.cso");
        auto psData = MyLib::BinaryFile::LoadFile(L"Resources/Shaders/ThermalScope_PS.cso");

        DX::ThrowIfFailed(device->CreateVertexShader(
            vsData->GetData(), vsData->GetSize(), nullptr,
            m_fullscreenVS.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(device->CreatePixelShader(
            psData->GetData(), psData->GetSize(), nullptr,
            m_thermalPS.ReleaseAndGetAddressOf()));

        // クロスヘア中心点
        Microsoft::WRL::ComPtr<ID3D11Resource>  res;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        m_crosshairTexture->GetResource(res.GetAddressOf());
        res.As(&tex);
        D3D11_TEXTURE2D_DESC desc{};
        tex->GetDesc(&desc);
        m_crosshairOrigin = Vector2(float(desc.Width) * 0.5f, float(desc.Height) * 0.5f);

        // スコープオーバーレイ用白ピクセルテクスチャ
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
                t.Get(), nullptr, m_scopeOverlayTexture.ReleaseAndGetAddressOf()));
        }
    }

    // ウィンドウサイズ依存リソース作成
    void TestScene::CreateWindowSizeDependentResources()
    {
        if (m_mag)
        {
            auto rect = GetUserResources()->GetDeviceResources()->GetOutputSize();
            m_mag->SetWindowSize(rect.right, rect.bottom);
        }
    }

    // 終了処理
    void TestScene::Finalize()
    {
        DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
        while (ShowCursor(TRUE) < 0) {}

        for (auto& inst : m_activeSoundInstances)
            if (inst) inst->Stop();
        m_activeSoundInstances.clear();

        m_shootSound.reset();
        // 環境音を停止
        if (m_windInstance)    m_windInstance->Stop();
        if (m_birdsInstance)   m_birdsInstance->Stop();
        if (m_cicadasInstance) m_cicadasInstance->Stop();

        m_windInstance.reset();
        m_birdsInstance.reset();
        m_cicadasInstance.reset();
        m_windSound.reset();
        m_birdsSound.reset();
        m_cicadasSound.reset();

        m_audioEngine.reset();
        m_bulletPool.clear();
        m_bulletCamTarget = nullptr;
        m_fpsGun.reset();
        m_player.reset();

        m_deers.clear();
        m_rabbits.clear();
        m_boss.reset();

        m_floor.reset();
        m_stage.reset();
        m_scope.reset();
        m_mag.reset();

        CameraManager::DeleteInstance();
    }

    // デバイス喪失時
    void TestScene::OnDeviceLost() {}

   

} // namespace MyLib
