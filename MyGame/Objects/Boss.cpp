#include <pch.h>
#include "Boss.h"
#include <iostream>
#include <float.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace MyLib;

// コンストラクタ：各メンバを初期化子リストで初期化（宣言順に合わせる）
Boss::Boss(const Vector3& pos, float radius)
    :
    m_position(pos),                                          // 位置
    m_radius(radius),                                         // 半径
    m_alive(true),                                            // 生存フラグ
    m_isWalking(true),                                        // 歩行中か
    m_showCollisionBox(false),                                // 判定ボックス表示
    m_facingAngle(0.0f),                                      // 向き
    m_debugCube(nullptr),                                     // デバッグキューブ
    m_bodyOffset(DirectX::SimpleMath::Vector3::Zero),         // 体オフセット
    m_headOffset(DirectX::SimpleMath::Vector3::Zero),         // 頭オフセット
    m_bodyCollider{},                                         // 体コライダー
    m_headCollider{},                                         // 頭コライダー
    m_headThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 頭サーマルオフセット
    m_killedByLethalShot(false),                              // 致命傷で倒されたか
    m_lungCollider{},                                         // 肺コライダー
    m_lungOffset(DirectX::SimpleMath::Vector3::Zero),         // 肺オフセット
    m_lungThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 肺サーマルオフセット
    m_hitCount(0),                                            // 被弾回数
    m_isCharging(false),                                      // 突進中か
    m_isAttacking(false),                                     // 攻撃中か
    m_lungHitOnce(false),                                     // 肺に当たったか
    m_chargeSpeed(8.0f),                                      // 突進速度
    m_attackRange(5.0f),                                      // 攻撃範囲
    m_attackTimer(0.0f),                                      // 攻撃タイマー
    m_attackDuration(0.97f),                                  // 攻撃時間
    m_hasAttacked(false),                                     // 攻撃済みか
    m_playerPosition(DirectX::SimpleMath::Vector3::Zero),     // プレイヤー位置
    m_chargeWayPoints{},                                      // 突進経路
    m_chargeWayPointIndex(0),                                 // 突進経路インデックス
    m_chargeTotalTime(0.0f),                                  // 突進経過時間
    m_chargeTransformRatio(0.0f),                             // 突進補間比率
    m_hasHitPlayer(false),                                    // プレイヤーに攻撃したか
    m_wayPoints{},                                            // 巡回経路
    m_totalTime(0.0f),                                        // 経過時間
    m_transformRatio(0.0f),                                   // 補間比率
    m_wayPointIndex(0),                                       // 経路インデックス
    m_model(nullptr),                                         // モデル
    m_anim_Idle(nullptr),                                     // 待機アニメ
    m_anim_Walk(nullptr),                                     // 歩行アニメ
    m_anim_Run(nullptr),                                      // 走行アニメ
    m_anim_Attack(nullptr),                                   // 攻撃アニメ
    m_currentAnim(nullptr),                                   // 現在のアニメ
    m_boneTransforms{},                                       // ボーン変換配列
    m_animState(AnimState::Walk),                             // アニメ状態
    m_animTimer(0.0f),                                        // アニメタイマー
    m_walkLoopTarget(4),                                      // 歩行ループ数
    m_idleDuration(2.0f),                                     // 待機時間
    m_deadTimer(0.0f),                                        // 死亡タイマー
    m_deadAnimDone(false)                                     // 死亡アニメ完了
{
}

Boss::~Boss() {}

// 初期化：コライダー（体・肺・頭）と巡回経路を設定
void Boss::Initialize(ID3D11DeviceContext* context)
{
    if (!context) return;

    // 当たり判定サイズ（体・肺・頭）
    Vector3 bodySize(m_radius * 0.6f, m_radius * 0.8f, m_radius * 0.9f);
    Vector3 lungSize(m_radius * 0.2f, m_radius * 0.4f, m_radius * 0.2f);
    Vector3 headSize(m_radius * 0.4f, m_radius * 0.4f, m_radius * 0.4f);

    m_debugCube = GeometricPrimitive::CreateCube(context);

    // 各部位の位置オフセット
   m_bodyOffset = Vector3(0.0f, 1.9f, -0.2f);
  // m_bodyOffset = Vector3(0.0f, 2.4f, -0.2f);
    m_headOffset = Vector3(0.0f, 2.1f, 1.3f);
    m_lungOffset = Vector3(0.4f, 2.0f, 0.3f);

    // OBBコライダー生成
    m_bodyCollider = OBBCollider(m_position + m_bodyOffset, bodySize * 0.5f, Matrix::Identity);
    m_headCollider = OBBCollider(m_position + m_headOffset, headSize * 0.5f, Matrix::Identity);
    m_lungCollider = OBBCollider(m_position + m_lungOffset, lungSize * 0.5f, Matrix::Identity);

    m_headThermalOffset = Vector3(0.0f, 0.3f, 0.0f);

    // 巡回経路（Catmull-Romの制御点）
    m_wayPoints =
    {
         Vector3(-10.0f, -1.0f, 25.0f),   // 開始点
         Vector3(-5.0f,  -1.0f, 20.0f),
         Vector3(0.0f,   -1.0f, 18.0f),
         Vector3(5.0f,   -1.0f, 20.0f),
         Vector3(10.0f,  -1.0f, 25.0f),
         Vector3(8.0f,   -1.0f, 30.0f),
         Vector3(0.0f,   -1.0f, 32.0f),
         Vector3(-8.0f,  -1.0f, 30.0f),
         Vector3(-10.0f, -1.0f, 25.0f),   // 開始点へ戻る
    };
    m_totalTime = 0.0f;
    m_transformRatio = 0.0f;
    m_wayPointIndex = 0;
}

// 更新：状態（攻撃・突進・巡回）に応じて動かす
void Boss::Update(float deltaTime)
{
    if (!m_alive) return;

    // 攻撃中：攻撃アニメが終わるまで待つ
    if (m_isAttacking)
    {
        m_attackTimer += deltaTime;
        if (m_attackTimer >= m_attackDuration)
        {
            m_isAttacking = false;
            m_attackTimer = 0.0f;
        }

        // コライダーを同期
        Matrix rot = Matrix::CreateRotationY(m_facingAngle);
        m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
        m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
        m_lungCollider.SetCenter(m_position + Vector3::Transform(m_lungOffset, rot));
        m_bodyCollider.SetOrientation(rot);
        m_headCollider.SetOrientation(rot);
        m_lungCollider.SetOrientation(rot);
        return;
    }

    // 突進中：プレイヤーへ向かって移動
    if (m_isCharging)
    {
        // プレイヤーに到達したら攻撃へ移行
        if (HasReachedPlayer())
        {
            m_isCharging = false;
            m_isAttacking = true;
            m_attackTimer = 0.0f;
            m_hasHitPlayer = true;
            OutputDebugStringA("BOSS: Reached player — attacking!\n");

            Matrix rot = Matrix::CreateRotationY(m_facingAngle);
            m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
            m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
            m_lungCollider.SetCenter(m_position + Vector3::Transform(m_lungOffset, rot));
            m_bodyCollider.SetOrientation(rot);
            m_headCollider.SetOrientation(rot);
            m_lungCollider.SetOrientation(rot);
            return;
        }

        // 突進経路に沿って移動（Catmull-Rom）
        if (m_chargeWayPoints.size() >= 4)
        {
            size_t  size = m_chargeWayPoints.size();
            Vector3 p0 = m_chargeWayPoints[std::min((size_t)std::max(m_chargeWayPointIndex - 1, 0), size - 1)];
            Vector3 p1 = m_chargeWayPoints[std::min((size_t)m_chargeWayPointIndex, size - 1)];
            Vector3 p2 = m_chargeWayPoints[std::min((size_t)m_chargeWayPointIndex + 1, size - 1)];
            Vector3 p3 = m_chargeWayPoints[std::min((size_t)m_chargeWayPointIndex + 2, size - 1)];

            float dist = (p2 - p1).Length();
            m_chargeTotalTime += deltaTime;
            m_chargeTransformRatio = dist > 0.0f
                ? m_chargeTotalTime * m_chargeSpeed / dist
                : 1.0f;

            m_position = Vector3::CatmullRom(p0, p1, p2, p3,
                std::min(m_chargeTransformRatio, 1.0f));

            // 進行方向を向く
            float   futureRatio = std::min(m_chargeTransformRatio + 0.01f, 1.0f);
            Vector3 futurePos = Vector3::CatmullRom(p0, p1, p2, p3, futureRatio);
            Vector3 forward = futurePos - m_position;
            if (forward.LengthSquared() > 0.0001f)
            {
                forward.Normalize();
                m_facingAngle = atan2f(forward.x, forward.z);
            }

            // 次の制御点へ。最後は直接プレイヤーへ向かう
            if (m_chargeTransformRatio >= 1.0f)
            {
                m_chargeWayPointIndex++;
                m_chargeTotalTime = 0.0f;

                if (m_chargeWayPointIndex >= (int)size - 1)
                {
                    Vector3 dir = m_playerPosition - m_position;
                    if (dir.LengthSquared() > 0.0001f)
                    {
                        dir.Normalize();
                        m_position += dir * m_chargeSpeed * deltaTime;
                        m_facingAngle = atan2f(dir.x, dir.z);
                    }
                }
            }
        }
        else
        {
            // 経路が無い場合：直線でプレイヤーへ突進
            Vector3 dir = m_playerPosition - m_position;
            if (dir.LengthSquared() > 0.0001f)
            {
                dir.Normalize();
                m_position += dir * m_chargeSpeed * deltaTime;
                m_facingAngle = atan2f(dir.x, dir.z);
            }
        }

        // コライダーを同期
        Matrix rot = Matrix::CreateRotationY(m_facingAngle);
        m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
        m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
        m_lungCollider.SetCenter(m_position + Vector3::Transform(m_lungOffset, rot));
        m_bodyCollider.SetOrientation(rot);
        m_headCollider.SetOrientation(rot);
        m_lungCollider.SetOrientation(rot);
        return;
    }

    // 通常の巡回
    if (m_wayPoints.size() < 4)
    {
        // 経路が無い場合：その場でコライダーだけ同期
        Matrix rot = Matrix::CreateRotationY(m_facingAngle);
        m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
        m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
        m_lungCollider.SetCenter(m_position + Vector3::Transform(m_lungOffset, rot));
        m_bodyCollider.SetOrientation(rot);
        m_headCollider.SetOrientation(rot);
        m_lungCollider.SetOrientation(rot);
        return;
    }

    // 巡回経路の4制御点を取得
    const size_t size = m_wayPoints.size();
    Vector3 p0 = m_wayPoints[(m_wayPointIndex + 0) % size];
    Vector3 p1 = m_wayPoints[(m_wayPointIndex + 1) % size];
    Vector3 p2 = m_wayPoints[(m_wayPointIndex + 2) % size];
    Vector3 p3 = m_wayPoints[(m_wayPointIndex + 3) % size];

    float distance = (p2 - p1).Length();
    m_totalTime += deltaTime;
    m_transformRatio = m_totalTime * 1.5f / distance;

    // Catmull-Romで滑らかに移動
    m_position = Vector3::CatmullRom(p0, p1, p2, p3, m_transformRatio);

    // 進行方向を向く
    float futureRatio = std::min(m_transformRatio + 0.01f, 1.0f);
    Vector3 futurePos = Vector3::CatmullRom(p0, p1, p2, p3, futureRatio);
    Vector3 forward = futurePos - m_position;
    if (forward.LengthSquared() > 0.0001f)
    {
        forward.Normalize();
        m_facingAngle = atan2f(forward.x, forward.z);
    }

    // 次の制御点へ進む
    if (m_transformRatio >= 1.0f)
    {
        m_wayPointIndex = (m_wayPointIndex + 1) % size;
        m_totalTime = 0.0f;
    }

    // コライダーを同期
    Matrix rot = Matrix::CreateRotationY(m_facingAngle);
    m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
    m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
    m_lungCollider.SetCenter(m_position + Vector3::Transform(m_lungOffset, rot));
    m_bodyCollider.SetOrientation(rot);
    m_headCollider.SetOrientation(rot);
    m_lungCollider.SetOrientation(rot);
}

// 描画：デバッグ用の当たり判定ボックス（体=緑・頭=赤・肺=黄）
void Boss::Render(const Matrix& view, const Matrix& proj)
{
    if (!m_alive || !m_showCollisionBox || !m_debugCube)
        return;

    Matrix bodyWorld =
        Matrix::CreateScale(m_bodyCollider.GetHalfSize() * 2.0f) *
        m_bodyCollider.GetOrientation() *
        Matrix::CreateTranslation(m_bodyCollider.GetCenter());
    m_debugCube->Draw(bodyWorld, view, proj, Colors::Green);

    Matrix headWorld =
        Matrix::CreateScale(m_headCollider.GetHalfSize() * 2.0f) *
        m_headCollider.GetOrientation() *
        Matrix::CreateTranslation(m_headCollider.GetCenter());
    m_debugCube->Draw(headWorld, view, proj, Colors::Red);

    Matrix lungWorld =
        Matrix::CreateScale(m_lungCollider.GetHalfSize() * 2.0f) *
        m_lungCollider.GetOrientation() *
        Matrix::CreateTranslation(m_lungCollider.GetCenter());
    m_debugCube->Draw(lungWorld, view, proj, Colors::Yellow);
}

// モデルとアニメーションの読み込み
void Boss::LoadModel(ID3D11Device* device)
{
    auto fx = std::make_unique<EffectFactory>(device);
    fx->SetDirectory(L"Resources/Textures");
    fx->SetSharing(false);

    // スキンメッシュモデルを読み込む
    m_model = Model::CreateFromSDKMESH(
        device,
        L"Resources/Models/Tiger/TigerIdle.sdkmesh",
        *fx,
        ModelLoader_IncludeBones
    );

    // 4種類のアニメーションを生成
    m_anim_Idle = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Walk = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Run = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Attack = std::make_unique<DX::AnimationSDKMESH>();

    // アニメーションファイルを読み込む
    DX::ThrowIfFailed(m_anim_Idle->Load(L"Resources/Models/Tiger/TigerIdle.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Walk->Load(L"Resources/Models/Tiger/TigerWalk.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Run->Load(L"Resources/Models/Tiger/TigerRun.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Attack->Load(L"Resources/Models/Tiger/TigerAttack.sdkmesh_anim"));

    // モデルにバインド
    m_anim_Idle->Bind(*m_model);
    m_anim_Walk->Bind(*m_model);
    m_anim_Run->Bind(*m_model);
    m_anim_Attack->Bind(*m_model);

    // ボーン行列を初期化
    m_boneTransforms = DirectX::ModelBone::MakeArray(m_model->bones.size());
    for (size_t i = 0; i < m_model->bones.size(); ++i)
        m_boneTransforms[i] = DirectX::XMMatrixIdentity();

    // 初期アニメは歩き
    m_currentAnim = m_anim_Walk.get();
    m_animState = AnimState::Walk;
}
Vector3 Boss::GetBodyCenter() const
{
    return m_bodyCollider.GetCenter();
}
// アニメーション更新：状態に応じてクリップを切り替える
void Boss::UpdateAnimation(float deltaTime)
{
    if (!m_model || !m_currentAnim) return;

    // 死亡時：攻撃アニメを死亡演出として一度だけ再生
    if (!m_alive && m_animState != AnimState::Attack)
    {
        m_animState = AnimState::Attack;
        m_currentAnim = m_anim_Attack.get();
        m_currentAnim->ResetAnimTime();
        m_currentAnim->ResetLoopCount();
        m_currentAnim->SetLoopOnce(true);
        m_deadTimer = 0.0f;
    }

    if (m_alive)
    {
        // 攻撃中 → 攻撃アニメ
        if (m_isAttacking && m_animState != AnimState::Attack)
        {
            m_animState = AnimState::Attack;
            m_currentAnim = m_anim_Attack.get();
            m_currentAnim->ResetAnimTime();
            m_currentAnim->ResetLoopCount();
        }
        // 突進中 → 走りアニメ
        else if (m_isCharging && m_animState != AnimState::Run)
        {
            m_animState = AnimState::Run;
            m_currentAnim = m_anim_Run.get();
            m_currentAnim->ResetAnimTime();
            m_currentAnim->ResetLoopCount();
        }
        // 通常時 → 歩き⇔待機の切り替え
        else if (!m_isCharging && !m_isAttacking)
        {
            m_animTimer += deltaTime;
            if (m_isWalking)
            {
                // 一定ループ歩いたら待機へ
                if (m_currentAnim->GetLoopCount() >= m_walkLoopTarget)
                {
                    m_isWalking = false;
                    m_animTimer = 0.0f;
                    m_animState = AnimState::Idle;
                    m_currentAnim = m_anim_Idle.get();
                    m_currentAnim->ResetAnimTime();
                    m_currentAnim->ResetLoopCount();
                }
            }
            else
            {
                // 待機が一定時間経ったら歩きへ
                if (m_currentAnim->GetLoopCount() >= 1 && m_animTimer >= m_idleDuration)
                {
                    m_isWalking = true;
                    m_animTimer = 0.0f;
                    m_animState = AnimState::Walk;
                    m_currentAnim = m_anim_Walk.get();
                    m_currentAnim->ResetAnimTime();
                    m_currentAnim->ResetLoopCount();
                }
            }
        }
    }
    else
    {
        // 死亡アニメ完了の判定
        m_deadTimer += deltaTime;
        if (m_deadTimer >= 2.0f)
            m_deadAnimDone = true;
    }

    // 再生速度（突進中は速め）
    float speed = m_isCharging ? 1.0f : 0.5f;
    m_currentAnim->Update(deltaTime * speed);

    // ボーン行列を計算して適用
    if (m_model->bones.size() > 0 && m_boneTransforms.get() != nullptr)
    {
        m_currentAnim->Apply(*m_model, m_model->bones.size(), m_boneTransforms.get());
        m_currentAnim->ApplySkinMatrix(*m_model, m_model->bones.size(), m_boneTransforms.get());
    }
}

// スキンメッシュ描画
void Boss::DrawModel(ID3D11DeviceContext* context, DirectX::CommonStates& states,
    const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& proj)
{
    if (!m_model) return;

    // ワールド行列（縮小・180度回転・向き・位置）
    Matrix world =
        Matrix::CreateScale(0.02f) *
        Matrix::CreateRotationY(XMConvertToRadians(180.0f)) *
        Matrix::CreateRotationY(m_facingAngle) *
        Matrix::CreateTranslation(m_position + Vector3(0.0f, 0.8f, 0.0f));

    m_model->DrawSkinned(
        context, states,
        m_model->bones.size(),
        m_boneTransforms.get(),
        world, view, proj
    );
}

// レイ判定：頭・肺・体のどこに当たったか判定
bool Boss::CheckRayHit(const Ray& ray, float& distance, HitPart& outPart)
{
    if (!m_alive) return false;   // 死亡時は判定しない

    // 各部位までの距離（初期値は最大）
    float dHead = FLT_MAX, dBody = FLT_MAX, dLung = FLT_MAX;

    // 各コライダーにレイが当たったか判定
    bool hitHead = IsHit(ray, m_headCollider, &dHead);
    bool hitBody = IsHit(ray, m_bodyCollider, &dBody);
    bool hitLung = IsHit(ray, m_lungCollider, &dLung);

    // どこにも当たらなければ命中なし
    if (!hitHead && !hitBody && !hitLung)
    {
        outPart = HitPart::NONE;
        return false;
    }

    // 優先順位：頭 → 肺 → 体
    if (hitHead)
    {
        distance = dHead;
        outPart = HitPart::HEAD;
    }
    else if (hitLung)
    {
        distance = dLung;
        outPart = HitPart::LUNG;
    }
    else
    {
        distance = dBody;
        outPart = HitPart::BODY;
    }

    return true;
}

// レイ命中時の処理
void Boss::OnRayHit(HitPart part)
{
    ApplyHit(part);
    OutputDebugStringA(
        part == HitPart::HEAD ? "BOSS HIT: HEAD\n" :
        part == HitPart::BODY ? "BOSS HIT: BODY\n" :
        part == HitPart::LUNG ? "BOSS HIT: LUNG\n" :
        "BOSS HIT: UNKNOWN\n"
    );
}

// ダメージ適用：2発で倒れる。1発目で突進開始
void Boss::ApplyHit(HitPart part)
{
    m_hitCount++;
    char buf[128];
    sprintf_s(buf, "BOSS HIT — hitCount=%d\n", m_hitCount);
    OutputDebugStringA(buf);

    if (m_hitCount >= 2)
    {
        // 2発目：撃破
        m_killedByLethalShot = true;
        Kill();
        OutputDebugStringA("BOSS: KILLED (2 hits)!\n");
    }
    else
    {
        // 1発目：突進開始
        m_killedByLethalShot = false;
        if (!m_isCharging && !m_isAttacking)
            StartCharging();
        OutputDebugStringA("BOSS: First hit — charging!\n");
    }
}

const Vector3& Boss::GetPosition() const
{
    return m_position;
}

// サーマル表示用：頭の中心位置
Vector3 Boss::GetHeadCenter() const
{
    return m_headCollider.GetCenter() + m_headThermalOffset;
}

// サーマル表示用：肺の中心位置
Vector3 Boss::GetLungCenter() const
{
    return m_lungCollider.GetCenter() + m_lungThermalOffset;
}

// 突進開始：突進経路を生成して走り状態へ
void Boss::StartCharging()
{
    m_isCharging = true;
    m_isWalking = false;
    m_isAttacking = false;
    GenerateChargeWaypoints();
    OutputDebugStringA("BOSS: Charging player!\n");
}

// 突進経路の生成：現在地からプレイヤーへ曲線パスを作る
void Boss::GenerateChargeWaypoints()
{
    Vector3 toPlayer = m_playerPosition - m_position;
    float   dist = toPlayer.Length();

    // 中間点を左右にずらして曲線的な突進にする
    Vector3 mid1 = m_position + toPlayer * 0.33f + Vector3(3.0f, 0.0f, 0.0f);
    Vector3 mid2 = m_position + toPlayer * 0.66f + Vector3(-3.0f, 0.0f, 0.0f);

    m_chargeWayPoints =
    {
        m_position,         // 開始（現在地）
        mid1,               // 中間1
        mid2,               // 中間2
        m_playerPosition,   // 目標（プレイヤー）
    };

    m_chargeWayPointIndex = 0;
    m_chargeTotalTime = 0.0f;
    m_chargeTransformRatio = 0.0f;
}

// プレイヤーに到達したか（攻撃範囲内か）判定
bool Boss::HasReachedPlayer() const
{
    float dist = Vector3::Distance(m_position, m_playerPosition);
    return dist <= m_attackRange;
}