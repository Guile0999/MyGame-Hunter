#include <pch.h>
#include "Rabbit.h"
#include <iostream>
#include <float.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace MyLib;

// コンストラクタ：各メンバを初期化子リストで初期化（宣言順に合わせる）
Rabbit::Rabbit(const Vector3& pos, float radius)
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
    m_bodyThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 体サーマルオフセット
    m_headThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 頭サーマルオフセット
    m_wayPoints{},                                            // 巡回経路
    m_totalTime(0.0f),                                        // 経過時間
    m_transformRatio(0.0f),                                   // 補間比率
    m_wayPointIndex(0),                                       // 経路インデックス
    m_model(nullptr),                                         // モデル
    m_anim_Idle(nullptr),                                     // 待機アニメ
    m_anim_Walk(nullptr),                                     // 歩行アニメ
    m_anim_Run(nullptr),                                      // 走行アニメ
    m_anim_Dead(nullptr),                                     // 死亡アニメ
    m_currentAnim(nullptr),                                   // 現在のアニメ
    m_boneTransforms{},                                       // ボーン変換配列
    m_animState(AnimState::Walk),                             // アニメ状態
    m_animTimer(0.0f),                                        // アニメタイマー
    m_walkLoopTarget(4),                                      // 歩行ループ数
    m_idleDuration(1.0f),                                     // 待機時間
    m_isDeadAnimating(false),                                 // 死亡アニメ中か
    m_deadTimer(0.0f),                                        // 死亡タイマー
    m_deadAnimDone(false),                                    // 死亡アニメ完了
    m_speedScale(1.0f),                                       // 速度倍率
    m_isSpooked(false),                                       // 逃走中か
    m_spookTimer(0.0f)                                        // 逃走タイマー
{
}
Rabbit::~Rabbit() {}

//初期化
void Rabbit::Initialize(ID3D11DeviceContext* context)
{
    if (!context) return;

    // // 当たり判定のサイズ（体・頭）
    Vector3 bodySize(m_radius * 0.5f, m_radius * 0.5f, m_radius * 0.7f);
    Vector3 headSize(m_radius * 0.2f, m_radius * 0.2f, m_radius * 0.2f);

    m_debugCube = GeometricPrimitive::CreateCube(context);

    //// 体・頭の位置オフセット
    m_bodyOffset = Vector3(0.1f, 1.0f, -0.1f);
    m_headOffset = Vector3(0.0f, 1.2f, 0.3f);

    // OBBコライダー生成
    m_bodyCollider = OBBCollider(
        m_position + m_bodyOffset,
        bodySize * 0.5f,
        Matrix::Identity
    );
    m_headCollider = OBBCollider(
        m_position + m_headOffset,
        headSize * 0.5f,
        Matrix::Identity
    );

    // サーマル表示用オフセット
    m_bodyThermalOffset = Vector3(0.0f, 0.1f, 0.0f);
    m_headThermalOffset = Vector3(0.0f, 0.1f, 0.0f);

    // 巡回経路はlevels.jsonから SetWaypoints() で設定される
    m_totalTime = 0.0f;
    m_transformRatio = 0.0f;
    m_wayPointIndex = 0;
}

//更新
void Rabbit::Update(float deltaTime)
{
    if (!m_alive) return;
    if (m_wayPoints.size() < 4) return;

    // 停止中：その場でコライダーだけ同期
    if (!m_isWalking)
    {
        Matrix rot = Matrix::CreateRotationY(m_facingAngle);
        m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
        m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
        m_bodyCollider.SetOrientation(rot);
        m_headCollider.SetOrientation(rot);
        return;
    }

    // 経路の4制御点を取得
    const size_t size = m_wayPoints.size();
    Vector3 p0 = m_wayPoints[(m_wayPointIndex + 0) % size];
    Vector3 p1 = m_wayPoints[(m_wayPointIndex + 1) % size];
    Vector3 p2 = m_wayPoints[(m_wayPointIndex + 2) % size];
    Vector3 p3 = m_wayPoints[(m_wayPointIndex + 3) % size];

    float distance = (p2 - p1).Length();
    m_totalTime += deltaTime;

    // 移動速度（驚いた時は2倍速で逃げる）
    float moveSpeed = m_isSpooked ? (2.5f * RUN_SPEED_SCALE) : 2.5f;
    m_transformRatio = m_totalTime * moveSpeed * m_speedScale / distance;

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
    // コライダーを位置・向きに合わせて同期
    Matrix rot = Matrix::CreateRotationY(m_facingAngle);
    m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));
    m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));
    m_bodyCollider.SetOrientation(rot);
    m_headCollider.SetOrientation(rot);
}

// 描画
void Rabbit::Render(const Matrix& view, const Matrix& proj)
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
}
// モデルとアニメーションの読み込み
void Rabbit::LoadModel(ID3D11Device* device)
{
    auto fx = std::make_unique<EffectFactory>(device);
    fx->SetDirectory(L"Resources/Textures");
    fx->SetSharing(false);

    // スキンメッシュモデルを読み込む
    m_model = Model::CreateFromSDKMESH(
        device,
        L"Resources/Models/Hare/HareIdle.sdkmesh",
        *fx,
        ModelLoader_IncludeBones
    );

    // 4種類のアニメーションを生成
    m_anim_Idle = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Walk = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Run = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Dead = std::make_unique<DX::AnimationSDKMESH>();

    // アニメーションファイルを読み込む
    DX::ThrowIfFailed(m_anim_Idle->Load(L"Resources/Models/Hare/HareIdle.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Walk->Load(L"Resources/Models/Hare/HareWalk.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Run->Load(L"Resources/Models/Hare/HareRun.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Dead->Load(L"Resources/Models/Hare/HareDead.sdkmesh_anim"));

    // モデルにバインド
    m_anim_Idle->Bind(*m_model);
    m_anim_Walk->Bind(*m_model);
    m_anim_Run->Bind(*m_model);
    m_anim_Dead->Bind(*m_model);

    //// ボーン行列を初期化
    m_boneTransforms = DirectX::ModelBone::MakeArray(m_model->bones.size());
    for (size_t i = 0; i < m_model->bones.size(); ++i)
        m_boneTransforms[i] = DirectX::XMMatrixIdentity();

    m_currentAnim = m_anim_Walk.get();
    m_animState = AnimState::Walk;
}

//// アニメーション更新
void Rabbit::UpdateAnimation(float deltaTime)
{
    if (!m_model || !m_currentAnim) return;

    // 死亡時：死亡アニメに一度だけ切り替え
    if (!m_alive && m_animState != AnimState::Dead)
    {
        m_animState = AnimState::Dead;
        m_currentAnim = m_anim_Dead.get();
        m_currentAnim->ResetAnimTime();
        m_currentAnim->ResetLoopCount();
        m_currentAnim->SetLoopOnce(true);
        m_isDeadAnimating = true;
        m_deadTimer = 0.0f;
    }

    // 生存中のアニメーション処理
    if (m_animState != AnimState::Dead)
    {
        if (m_isSpooked)
        {
            // 逃走中：一定時間走ってから落ち着く
            m_spookTimer += deltaTime;
            if (m_spookTimer >= SPOOK_DURATION)
            {
                // 落ち着いて歩きに戻る
                m_isSpooked = false;
                m_animState = AnimState::Walk;
                m_currentAnim = m_anim_Walk.get();
                m_currentAnim->ResetAnimTime();
                m_currentAnim->ResetLoopCount();
                m_animTimer = 0.0f;
                m_isWalking = true;
            }
            
        }
        else
        {
            // 通常の歩き⇔待機の切り替え
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

    // 再生速度（死亡=遅め・逃走=速め・通常）
    float speed;
    if (m_animState == AnimState::Dead)  speed = 0.6f;
    else if (m_isSpooked)                speed = 1.0f;
    else                                 speed = 0.7f;

    m_currentAnim->Update(deltaTime * speed);

    // ボーン行列を計算して適用
    if (m_model->bones.size() > 0 && m_boneTransforms.get() != nullptr)
    {
        m_currentAnim->Apply(*m_model, m_model->bones.size(), m_boneTransforms.get());
        m_currentAnim->ApplySkinMatrix(*m_model, m_model->bones.size(), m_boneTransforms.get());
    }
}

// スキンメッシュ描画
void Rabbit::DrawModel(ID3D11DeviceContext* context, DirectX::CommonStates& states,
    const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& proj)
{
    if (!m_model) return;

    //// ワールド行列（縮小・向き・位置）
    Matrix world =
        Matrix::CreateScale(0.02f) *
        Matrix::CreateRotationY(m_facingAngle) *
        Matrix::CreateTranslation(m_position + Vector3(0.0f, 0.8f, 0.0f));

    m_model->DrawSkinned(
        context, states,
        m_model->bones.size(),
        m_boneTransforms.get(),
        world, view, proj
    );
}

// レイ判定
bool Rabbit::CheckRayHit(const Ray& ray, float& distance, HitPart& outPart)
{
    if (!m_alive) return false;

    // 頭・体までの距離（初期値は最大）
    float dHead = FLT_MAX;
    float dBody = FLT_MAX;

    // 頭・体それぞれにレイが当たったか判定
    bool hitHead = IsHit(ray, m_headCollider, &dHead);
    bool hitBody = IsHit(ray, m_bodyCollider, &dBody);

    // どちらにも当たらなければ命中なし
    if (!hitHead && !hitBody)
    {
        outPart = HitPart::NONE;
        return false;
    }

    // より近い方を採用（頭優先）
    if (hitHead && dHead <= dBody)
    {
        distance = dHead;
        outPart = HitPart::HEAD;
    }
    else
    {
        distance = dBody;
        outPart = HitPart::BODY;
    }

    return true;
}

// レイ命中時の処理
void Rabbit::OnRayHit(HitPart part)
{
    ApplyHit(part);
    OutputDebugStringA(
        part == HitPart::HEAD ? "RABBIT HIT: HEAD\n" :
        part == HitPart::BODY ? "RABBIT HIT: BODY\n" :
        "RABBIT HIT: UNKNOWN\n"
    );
}

// ダメージ適用
void Rabbit::ApplyHit(HitPart part)
{
    Kill();
}

const Vector3& Rabbit::GetPosition() const
{
    return m_position;
}

// 移動のばらつき設定（開始位置と速度）
void Rabbit::SetMovementVariation(int startIndex, float speedScale)
{
    m_wayPointIndex = startIndex;
    m_speedScale = speedScale;
}

// 驚かせる：走りアニメに切り替えて逃走開始
void MyLib::Rabbit::rabbitSpook()
{
    if (!m_alive) return;
    if (m_animState == AnimState::Dead) return;

    m_isSpooked = true;
    m_spookTimer = 0.0f;

    m_animState = AnimState::Run;
    m_currentAnim = m_anim_Run.get();
    m_currentAnim->ResetAnimTime();
    m_currentAnim->ResetLoopCount();
    m_isWalking = true;
}

// サーマル表示用：体の中心位置
Vector3 Rabbit::GetBodyCenter() const
{
    return m_bodyCollider.GetCenter() + m_bodyThermalOffset;
}

// サーマル表示用：頭の中心位置
Vector3 Rabbit::GetHeadCenter() const
{
    return m_headCollider.GetCenter() + m_headThermalOffset;
}