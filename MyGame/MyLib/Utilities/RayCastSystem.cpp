#include "pch.h"
#include "RaycastSystem.h"
#include <DirectXCollision.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    // 射撃可能なオブジェクトを追加（所有しない）
    void RaycastSystem::AddTarget(IRaycastTarget* target)
    {
        m_targets.push_back(target);
    }

    // 登録済みターゲットを全て消す
    void RaycastSystem::ClearTargets()
    {
        m_targets.clear();
    }

    // レイを飛ばして最初に当たった対象を検出
    bool RaycastSystem::ShootRay(
        const Ray& ray,
        float& outDistance,
        IRaycastTarget*& outTarget,
        IRaycastTarget::HitPart& outHitPart)
    {
        outTarget = nullptr;
        outHitPart = IRaycastTarget::HitPart::NONE;

        bool hitAnything = false;
        float closestDist = FLT_MAX;
        IRaycastTarget* closestTarget = nullptr;
        IRaycastTarget::HitPart closestPart = IRaycastTarget::HitPart::NONE;

        // 全ターゲットに対してレイをチェック
        for (auto* target : m_targets)
        {
            if (!target) continue;   // 念のためnullチェック

            float distance = 0.0f;
            IRaycastTarget::HitPart hitPart = IRaycastTarget::HitPart::NONE;

            if (target->CheckRayHit(ray, distance, hitPart))
            {
                if (distance > 0.0001f && distance < closestDist)
                {
                    closestDist = distance;
                    closestTarget = target;
                    closestPart = hitPart;
                    hitAnything = true;
                }
            }
        }

        if (hitAnything)
        {
            outDistance = closestDist;
            outTarget = closestTarget;
            outHitPart = closestPart;
            return true;
        }
        return false;
    }
}