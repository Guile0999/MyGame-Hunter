#include <pch.h>
#include "Deer.h"
#include <iostream>
#include <float.h>
#include "Rabbit.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace MyLib;

// 鹿クラスの実装
// コンストラクタ：各メンバを初期化子リストで初期化（宣言順に合わせる）
Deer::Deer(const Vector3& pos, float radius)
    :
    m_position(pos),                                          // 位置
    m_radius(radius),                                         // 半径
    m_alive(true),                                            // 生存フラグ
    m_debugCube(nullptr),                                     // デバッグキューブ
    m_bodyBox(nullptr),                                       // 体ボックス
    m_headBox(nullptr),                                       // 頭ボックス
    m_basePosition(pos),                                      // 待機時の基準位置
    m_bodyOffset(DirectX::SimpleMath::Vector3::Zero),         // 体オフセット
    m_headOffset(DirectX::SimpleMath::Vector3::Zero),         // 頭オフセット
    m_bodyCollider{},                                         // 体コライダー
    m_headCollider{},                                         // 頭コライダー
    m_bodyThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 体サーマルオフセット
    m_headThermalOffset(DirectX::SimpleMath::Vector3::Zero),  // 頭サーマルオフセット
    m_showCollisionBox(true),                                 // 判定ボックス表示
    m_isWalking(true),                                        // 歩行中か
    m_wayPoints{},                                            // 巡回経路
    m_totalTime(0.0f),                                        // 経過時間
    m_transformRatio(0.0f),                                   // 補間比率
    m_wayPointIndex(0),                                       // 経路インデックス
    m_facingAngle(0.0f),                                      // 向き
    m_animatedHeadPos(DirectX::SimpleMath::Vector3::Zero),    // アニメ頭位置
    m_hasAnimatedHead(false),                                 // アニメ頭ありフラグ
    m_model(nullptr),                                         // モデル
    m_anim_Idle(nullptr),                                     // 待機アニメ
    m_anim_Walk(nullptr),                                     // 歩行アニメ
    m_anim_Run(nullptr),                                      // 走行アニメ
    m_anim_Dead(nullptr),                                     // 死亡アニメ
    m_currentAnim(nullptr),                                   // 現在のアニメ
    m_boneTransforms{},                                       // ボーン変換配列
    m_isSpooked(false),                                       // 逃走中か
    m_spookTimer(0.0f),                                       // 逃走タイマー
    m_animState(AnimState::Walk),                             // アニメ状態
    m_animTimer(0.0f),                                        // アニメタイマー
    m_walkLoopTarget(6),                                      // 歩行ループ数
    m_idleDuration(2.0f),                                     // 待機時間
    m_isDeadAnimating(false),                                 // 死亡アニメ中か
    m_deadTimer(0.0f),                                        // 死亡タイマー
    m_deadAnimDone(false),                                    // 死亡アニメ完了
    m_speedScale(1.0f)                                        // 速度倍率
{
}
// デストラクタ
Deer::~Deer() {}

// 初期化
void Deer::Initialize(ID3D11DeviceContext* context)
{
    if (!context) return;

	//頭と胴体のサイズ（コリジョン用）
    Vector3 bodySize(m_radius * 1.0f, m_radius * 1.0f, m_radius * 2.0f);
    Vector3 headSize(m_radius * 0.6f, m_radius * 0.6f, m_radius * 0.6f);

	// 衝突判定を可視化するためのデバッグキューブ
    m_debugCube = GeometricPrimitive::CreateCube(context);
   

	// 体と頭のオフセット（コリジョン用）
    m_bodyOffset = Vector3(0.0f, 1.8f, 0.0f);
    m_headOffset = Vector3(0.0f, 2.6f, 0.9f);
     

	// OBBコリジョンの初期化
    m_bodyCollider = OBBCollider(
        m_position + m_bodyOffset + Vector3(0.0f, 2.0f, 0.0f),
        bodySize * 0.5f,
        Matrix::Identity
    );

    m_headCollider = OBBCollider(
        m_position + m_headOffset + Vector3(0.0f, 2.0f, 0.0f),
        headSize * 0.5f,
        Matrix::Identity
    );

    m_bodyThermalOffset = Vector3(0.5f, 0.0f, 0.0f);
    m_headThermalOffset = Vector3(0.3f, 0.1f, 0.0f);
    

    m_basePosition = m_position;

    //経路はJSONから SetWaypoints() で設定される
    m_totalTime = 0.0f;
    m_transformRatio = 0.0f;
    m_wayPointIndex = 0;
}

// 更新
void Deer::Update(float deltaTime)
{
    

    if (!m_alive) return;
    if (m_wayPoints.size() < 4) return;

    // 歩行時のみ移動
    if (!m_isWalking)
    {
        
        if (!m_isWalking)
        {
			//  歩行していない場合は位置を固定して回転のみ更新
            Matrix rot = Matrix::CreateRotationY(m_facingAngle);
            Vector3 rotatedHeadOffset = Vector3::Transform(m_headOffset, rot);
            Vector3 rotatedBodyOffset = Vector3::Transform(m_bodyOffset, rot);

			//  コリジョンの位置と向きを更新
            m_bodyCollider.SetCenter(m_position + rotatedBodyOffset);
            m_headCollider.SetCenter(m_position + rotatedHeadOffset);
            m_bodyCollider.SetOrientation(rot);
            m_headCollider.SetOrientation(rot);
            return;
        }

		//  歩行していない場合は位置を固定して回転のみ更新

       // 歩行時の回転修正は同じ
        Matrix rot = Matrix::CreateRotationY(m_facingAngle);
        Vector3 rotatedHeadOffset = Vector3::Transform(m_headOffset, rot);
        Vector3 rotatedBodyOffset = Vector3::Transform(m_bodyOffset, rot);

		//  コリジョンの位置と向きを更新
        m_bodyCollider.SetCenter(m_position + rotatedBodyOffset);
        m_headCollider.SetCenter(m_position + rotatedHeadOffset);
        m_bodyCollider.SetOrientation(rot);
        m_headCollider.SetOrientation(rot);
    }


    // // キャットマル・ロムの4つの制御点を取得する
    const size_t size = m_wayPoints.size();
    Vector3 p0 = m_wayPoints[(m_wayPointIndex + 0) % size];
    Vector3 p1 = m_wayPoints[(m_wayPointIndex + 1) % size];
    Vector3 p2 = m_wayPoints[(m_wayPointIndex + 2) % size];
    Vector3 p3 = m_wayPoints[(m_wayPointIndex + 3) % size];

    // p1とp2間の距離を計算して移動速度を決定する。
    float distance = (p2 - p1).Length();
    float moveSpeed = m_isSpooked ? (1.2f * RUN_SPEED_SCALE) : 1.2f;
    // 蓄積時間ではなく、毎フレーム進行度を加算する（速度変化でワープしない）
    m_transformRatio += (moveSpeed * m_speedScale * deltaTime) / distance;
	// 0.0f 〜 1.0fの範囲にクランプ
    m_position = Vector3::CatmullRom(p0, p1, p2, p3, m_transformRatio);

	//  将来の位置を計算して、そこから向きを決定する
    float futureRatio = std::min(m_transformRatio + 0.01f, 1.0f);
    Vector3 futurePos = Vector3::CatmullRom(p0, p1, p2, p3, futureRatio);
    Vector3 forward = futurePos - m_position;

    if (forward.LengthSquared() > 0.0001f)
    {
        forward.Normalize();
		// atan2fはX軸とZ軸の比率から角度を計算する関数
        m_facingAngle = atan2f(forward.x, forward.z);
    }


	//  キャットマル・ロムの移動が完了したら、次の制御点セットに進む
    if (m_transformRatio >= 1.0f)
    {
        m_wayPointIndex = (m_wayPointIndex + 1) % size;
        m_transformRatio = 0.0f;
    }

	//  コリジョンの位置と向きを更新
    Matrix rot = Matrix::CreateRotationY(m_facingAngle);

    m_bodyCollider.SetCenter(m_position + Vector3::Transform(m_bodyOffset, rot));

    if (m_hasAnimatedHead)
        m_headCollider.SetCenter(m_animatedHeadPos);
    else
        m_headCollider.SetCenter(m_position + Vector3::Transform(m_headOffset, rot));

    m_bodyCollider.SetOrientation(rot);
    m_headCollider.SetOrientation(rot);
}

// デバッグ用のコリジョンボックスを描画
void Deer::Render(const Matrix& view, const Matrix& proj)
{
	//  デバッグ用のコリジョンボックスを描画
    if (!m_alive || !m_showCollisionBox || !m_debugCube)
        return;
	//  コリジョンの位置と向きを更新
    Matrix bodyWorld =
        Matrix::CreateScale(m_bodyCollider.GetHalfSize() * 2.0f) *
        m_bodyCollider.GetOrientation() *
        Matrix::CreateTranslation(m_bodyCollider.GetCenter());

    
    m_debugCube->Draw(bodyWorld, view, proj, Colors::Green);

	// 頭のコリジョンは赤で描画
    Matrix headWorld =
        Matrix::CreateScale(m_headCollider.GetHalfSize() * 2.0f) *
        m_headCollider.GetOrientation() *
        Matrix::CreateTranslation(m_headCollider.GetCenter());

    m_debugCube->Draw(headWorld, view, proj, Colors::Red);
}

// モデルとアニメーションの読み込み
void MyLib::Deer::LoadModel(ID3D11Device* device)
{
    auto fx = std::make_unique<EffectFactory>(device);
    fx->SetDirectory(L"Resources/Textures");
    fx->SetSharing(false);

	//モデルの読み込み。SDKMESH形式で、ボーン情報も含まれているものを指定
    m_model = Model::CreateFromSDKMESH(
        device,
        L"Resources/Models/Deer/deer_bone.sdkmesh",
        *fx,
        ModelLoader_IncludeBones
    );

	// アニメーションの読み込み。SDKMESH形式で、ボーン情報も含まれているものを指定
    m_anim_Walk = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Idle = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Run = std::make_unique<DX::AnimationSDKMESH>();
    m_anim_Dead = std::make_unique<DX::AnimationSDKMESH>();

	// アニメーションファイルは、モデルと同じSDKMESH形式で、ボーン情報も含まれているものを指定
    DX::ThrowIfFailed(m_anim_Walk->Load(L"Resources/Models/Deer/deer_walk.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Idle->Load(L"Resources/Models/Deer/deer_idle.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Run->Load(L"Resources/Models/Deer/deer_run.sdkmesh_anim"));
    DX::ThrowIfFailed(m_anim_Dead->Load(L"Resources/Models/Deer/deerDead.sdkmesh_anim"));

	// モデルとアニメーションをバインド（モデルのボーン構造にアニメーションを紐づける）
    m_anim_Walk->Bind(*m_model);
    m_anim_Idle->Bind(*m_model);
    m_anim_Run->Bind(*m_model);
    m_anim_Dead->Bind(*m_model);

	// ボーンの数に合わせて、ボーン変換行列の配列を確保
    m_boneTransforms = DirectX::ModelBone::MakeArray(m_model->bones.size());
    for (size_t i = 0; i < m_model->bones.size(); ++i)
        m_boneTransforms[i] = DirectX::XMMatrixIdentity();

    m_currentAnim = m_anim_Walk.get();
    m_animState = AnimState::Walk;
}

// アニメーションの更新と適用
void MyLib::Deer::UpdateAnimation(float deltaTime)
{
	// モデルとアニメーションがロードされていない場合は何もしない
    if (!m_model || !m_currentAnim) return;

	// 死亡している場合は死のアニメーションに切り替える
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

	// 死亡していない場合は、怯え状態かどうかでアニメーションを切り替える
    if (m_animState != AnimState::Dead)
    {
        if (m_isSpooked)
        {
            // 怯えている場合 — 怯えタイマーをカウントダウン
            m_spookTimer += deltaTime;
            if (m_spookTimer >= SPOOK_DURATION)
            {
                // 落ち着く — 歩行に戻る
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
			// 通常の歩行/待機アニメーションの切り替え
            m_animTimer += deltaTime;

            if (m_isWalking)
            {
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
				// 待機アニメーションが1ループ以上再生され、かつ待機時間を超えたら、歩行に戻る
                if (m_currentAnim->GetLoopCount() >= 1 &&
                    m_animTimer >= m_idleDuration)
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
		// 死亡アニメーションが完了したかどうかをチェック
        m_deadTimer += deltaTime;
        if (m_deadTimer >= 3.0f)
            m_deadAnimDone = true;
    }

	// アニメーションの更新速度を、状態に応じて変える（死んでいるときはゆっくり、怯えているときは速く）
    float speed;
    if (m_animState == AnimState::Dead)  speed = 0.6f;
    else if (m_isSpooked)                speed = 0.8f;
    else                                 speed = 0.5f;

    m_currentAnim->Update(deltaTime * speed);

    if (m_model->bones.size() > 0 && m_boneTransforms.get() != nullptr)
    {
        m_currentAnim->Apply(*m_model, m_model->bones.size(), m_boneTransforms.get());

		// 頭の位置を計算して、頭のコリジョンに反映させる
        if (m_model->bones.size() > HEAD_BONE_INDEX)
        {
            Vector3 headLocal = Matrix(m_boneTransforms[HEAD_BONE_INDEX]).Translation();

            Matrix world =
                Matrix::CreateScale(0.01f) *
                Matrix::CreateRotationY(m_facingAngle) *
                Matrix::CreateTranslation(m_position + Vector3(0.0f, 2.0f, 0.0f));

            m_animatedHeadPos = Vector3::Transform(headLocal, world);
            m_hasAnimatedHead = true;

        }

        m_currentAnim->ApplySkinMatrix(*m_model, m_model->bones.size(), m_boneTransforms.get());
    }
}
// モデルの描画
void MyLib::Deer::DrawModel(ID3D11DeviceContext* context, DirectX::CommonStates& states, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	// モデルとアニメーションがロードされていない場合は何もしない
    if (!m_model) return;

	// ワールド行列を作成（スケール、回転、平行移動）
    Matrix world =
        Matrix::CreateScale(0.01f) *
        Matrix::CreateRotationY(m_facingAngle) *
        Matrix::CreateTranslation(m_position + Vector3(0.0f, 2.0f, 0.0f));

	// スキニングを適用してモデルを描画
    m_model->DrawSkinned(
        context, states,
        m_model->bones.size(),
        m_boneTransforms.get(),
        world, view, proj
    );
}



// レイキャスト
bool Deer::CheckRayHit(const Ray& ray, float& distance, HitPart& outPart)
{
	// 死んでいる場合はヒットしない
    if (!m_alive)
        return false;

	// 頭と胴体の両方に対してレイとの交差判定を行い、ヒットした場合は距離を取得する
    float dHead = FLT_MAX;
    float dBody = FLT_MAX;

	//  交差判定の結果と距離をもとに、どの部位がヒットしたかを判断する
    bool hitHead = IsHit(ray, m_headCollider, &dHead);
    bool hitBody = IsHit(ray, m_bodyCollider, &dBody);

	// どちらもヒットしなかった場合は、ヒットなしとする
    if (!hitHead && !hitBody)
    {
        outPart = HitPart::NONE;
        return false;
    }

	// 頭と胴体の両方がヒットした場合は、距離が近い方を優先する
    if (hitHead && dHead <= dBody)
    {
        distance = dHead;
        outPart = HitPart::HEAD;
    }
	// 胴体がヒットし、頭よりも距離が近い場合は、胴体ヒットとする
    else
    {
        distance = dBody;
        outPart = HitPart::BODY;
    }

    return true;
}

// ヒットしたときの反応
void Deer::OnRayHit(HitPart part)
{
	// ヒット部位に応じた反応をする
    ApplyHit(part);
	// デバッグ用の出力
    OutputDebugStringA(
        part == HitPart::HEAD ? "DEER HIT: HEAD\n" :
        part == HitPart::BODY ? "DEER HIT: BODY\n" :
        "DEER HIT: UNKNOWN\n"
    );
}
// ミスしたときの反応 — 怯えて逃げる
void MyLib::Deer::Spook()
{
	// すでに死んでいる場合や、死のアニメーション中の場合は何もしない
    if (!m_alive) return;
    if (m_animState == AnimState::Dead) return;

	// 怯える状態に入る
    m_isSpooked = true;
    m_spookTimer = 0.0f;

	// 怯えアニメーションに切り替える
    m_animState = AnimState::Run;
    m_currentAnim = m_anim_Run.get();
    m_currentAnim->ResetAnimTime();
    m_currentAnim->ResetLoopCount();
    m_isWalking = true;   // keep moving while fleeing
    
}

// ヒット部位に応じた反応をする
void Deer::ApplyHit(HitPart part)
{
	// すでに死んでいる場合は何もしない
    Kill();

	// ヒット部位に応じた反応をする
    switch (part)
    {
    case HitPart::HEAD:
        std::cout << "Headshot!" << std::endl;
        break;
    case HitPart::BODY:
        std::cout << "Body shot!" << std::endl;
        break;
    default:
        break;
    }
}

// その他のメンバ関数
const Vector3& Deer::GetPosition() const
{
	return m_position; // 現在の位置を返す
}

// 位置を設定する関数。コリジョンの位置も更新する。
void Deer::SetPosition(const Vector3& pos)
{
	// 位置を更新
    m_position = pos;
    m_basePosition = pos;
    m_bodyCollider.SetCenter(m_position + m_bodyOffset);
    m_headCollider.SetCenter(m_position + m_headOffset);
}

// 半径を返す関数
float Deer::GetRadius() const { return m_radius; }

// 生存状態を返す関数
bool Deer::IsAlive() const { return m_alive; }

// 死亡させる関数
void Deer::Kill() { m_alive = false; }

// 移動のバリエーションを設定する関数。開始インデックスと速度スケールを指定する。
void MyLib::Deer::SetMovementVariation(int startIndex, float speedScale)
{
    m_wayPointIndex = startIndex;
    m_speedScale = speedScale;
}

// 体の中心位置を返す関数。コリジョンの中心に熱源オフセットを加えた位置を返す。
DirectX::SimpleMath::Vector3 MyLib::Deer::GetBodyCenter() const
{
    return m_bodyCollider.GetCenter() + m_bodyThermalOffset;
}

// 頭の中心位置を返す関数。コリジョンの中心に熱源オフセットを加えた位置を返す。
DirectX::SimpleMath::Vector3 MyLib::Deer::GetHeadCenter() const
{
    return m_headCollider.GetCenter() + m_headThermalOffset;

}
