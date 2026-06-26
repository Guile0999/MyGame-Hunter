//========================================================================================================================
// RaycastSystem.h
//
// レイでターゲットにできるオブジェクトのコレクションを管理するクラス。
// ※ターゲットは「所有しない」。生ポインタで参照するだけ（所有権はTestScene側にある）。
//========================================================================================================================
#pragma once
#include <pch.h>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <MyLib/Utilities/IRaycastTarget.h>
#include <MyLib/Utilities/Ray.h>

namespace MyLib {
    class RaycastSystem
    {
    public:
        RaycastSystem() = default;

        // 射撃可能なオブジェクトを追加（所有しない＝生ポインタ）
        void AddTarget(IRaycastTarget* target);

        // 登録済みターゲットを全て消す（ステージ切替などで使用）
        void ClearTargets();

        // レイを飛ばして最初に当たった対象を検出
        bool ShootRay(
            const Ray& ray,
            float& outDistance,
            IRaycastTarget*& outTarget,
            IRaycastTarget::HitPart& outHitPart);

    private:
        // 生ポインタ（観測のみ・所有しない）
        std::vector<IRaycastTarget*> m_targets;
    };
}