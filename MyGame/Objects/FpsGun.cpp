#include "pch.h"
#include "FpsGun.h"
#include "Objects/FirstPersonPlayer.h"
#include "MyLib/Utilities/RaycastSystem.h"
#include "MyLib/Utilities/IRaycastTarget.h"
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
     // コンストラクタ
    FpsGun::FpsGun()
        : m_world(Matrix::Identity) {}

    //銃の初期化
    void FpsGun::Initialize(const FpsGunSetting& setting)
    {
        m_setting = setting;
        if (m_setting.scale.x == 0) m_setting.scale.x = 1.0f;
        if (m_setting.scale.y == 0) m_setting.scale.y = 1.0f;
        if (m_setting.scale.z == 0) m_setting.scale.z = 1.0f;
    }
    //銃の更新
    void FpsGun::Update(const FirstPersonPlayer& player)
    {
        DirectX::SimpleMath::Vector3 cameraPos = player.GetCamera().GetPosition();
        Quaternion cameraRot = player.GetRotation();

        DirectX::SimpleMath::Vector3 offsetTransformed = DirectX::SimpleMath::Vector3::Transform(m_setting.offset, Matrix::CreateFromQuaternion(cameraRot));
        DirectX::SimpleMath::Vector3 worldPos = cameraPos + offsetTransformed;

        // 銃固有の追加回転
        Quaternion extraRotation = Quaternion::CreateFromYawPitchRoll(
            XMConvertToRadians(m_setting.rotation.y),
            XMConvertToRadians(m_setting.rotation.x),
            XMConvertToRadians(m_setting.rotation.z)
        );

        DirectX::SimpleMath::Matrix gunWorld =
            Matrix::CreateScale(m_setting.scale) *
            Matrix::CreateFromQuaternion(extraRotation) *
            Matrix::CreateTranslation(m_setting.offset);
        //calculate fpscamera and gun rotation
        m_world = gunWorld * 
            player.GetCamera().GetRotationMatrix() *
            Matrix::CreateTranslation(cameraPos);
    }

    Bullet FpsGun::Shoot(const FirstPersonPlayer& player,
        std::shared_ptr<DirectX::Model> bulletModel,
        RaycastSystem& raycaster) 
    {
       //カメラ位置・前方方向を取得
        const auto& camera = player.GetCamera();
        Vector3 cameraPos = camera.GetPosition();
        Vector3 cameraDir = camera.GetForward();

        // カメラからレイを作成
        Ray targetingRay(cameraPos, cameraDir);

        // Variables to store the result of the raycast
        float hitDistance = 0.0f;
        IRaycastTarget* hitTarget = nullptr;
        IRaycastTarget::HitPart hitPart = IRaycastTarget::HitPart::NONE;

        // レイキャストでターゲットを取得
      
        Vector3 targetPoint;
        if (raycaster.ShootRay(targetingRay, hitDistance, hitTarget, hitPart) && hitDistance < m_fireRange)
        {
            targetPoint = cameraPos + cameraDir * hitDistance;

        }
        else
        {
            //  射程範囲外の場合は遠方に飛ばす
            targetPoint = cameraPos + cameraDir * m_fireRange;
        }

        // 弾の開始位置を銃口から取得
        Vector3 muzzlePos = GetMuzzlePositionInWorld();

        //銃口からターゲットまでの方向を計算
        /*Vector3 finalBulletDirection = targetPoint - muzzlePos;
        finalBulletDirection.Normalize();*/
        Vector3 bulletDir = targetPoint - cameraPos;
        bulletDir.Normalize();

        // 弾を生成して返す
        //return Bullet(muzzlePos, finalBulletDirection, m_fireRange, bulletModel);
        return Bullet(cameraPos, bulletDir, m_fireRange, bulletModel);
    }

    DirectX::SimpleMath::Vector3 FpsGun::GetForward() const
    {
        return m_world.Forward();
    }

    DirectX::SimpleMath::Vector3 FpsGun::GetMuzzlePositionInWorld() const
    {
        // 銃口のワールド座標を取得（ライフルモデルに合わせてオフセット調整）
       const DirectX::SimpleMath::Vector3 muzzleOffset = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.5f);//(X,Y,Z)
        
        return DirectX::SimpleMath::Vector3::Transform(muzzleOffset, m_world);
    }
}