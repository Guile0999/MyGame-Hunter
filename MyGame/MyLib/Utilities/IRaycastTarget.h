//==========================================================================================================
// IRaycastTarget.h
//
// このインターフェースは、レイに当たることのできるオブジェクトを定義します。
// このインターフェースを実装するクラスは以下を行う必要があります：
// 1. OnRayHit() によってヒット時の反応を処理する。
// 2. CheckRayHit() でレイとの衝突判定を提供する。
// 
// また、オブジェクトのどの部分がヒットしたか（NONE, BODY, HEAD など）も定義します。
// 要するに、RaycastSystem がシーン内のさまざまなオブジェクトへのヒットを検出できるようにします。
//===========================================================================================================

#pragma once
#include <pch.h>
#include <SimpleMath.h>
#include <DirectXCollision.h>
#include <MyLib/Utilities/Ray.h>

namespace MyLib
{
    class Ray; // forward declaration

    // レイに当たることができるオブジェクト用のインターフェース
    class IRaycastTarget
    {
    public:
        virtual ~IRaycastTarget() = default;

        // オブジェクトのヒット部位
        enum class HitPart
        {
            NONE,    // ヒットなし
            BODY,   // 胴体
            HEAD,   // 頭
            LUNG    // 肺


        };

       

        // オブジェクトがヒットしたときに呼ばれる
        virtual void OnRayHit(HitPart part) = 0;

        // レイがこのオブジェクトに当たるかチェック
        virtual bool CheckRayHit(const Ray& ray, float& distance, HitPart& outPart) = 0;
    };
}