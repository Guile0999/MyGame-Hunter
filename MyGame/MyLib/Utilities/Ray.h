//===================================================================
// Ray.h
//
// このクラスは、原点(origin)と方向(direction)を持つ3Dレイを表します。
// レイは3D空間での交差判定に使用されます。例えば：
// - 発射や視線判定
// - マウスによるオブジェクト選択
// - 物理衝突判定
//
// また、バウンディングボックスとの交差判定用のメソッドも提供します。
// 方向ベクトルは常に正規化されます。
//====================================================================

#pragma once
#include <pch.h>
#include <SimpleMath.h>
#include <DirectXCollision.h>

namespace MyLib
{
    class Ray
    {
    public:
        DirectX::SimpleMath::Vector3 origin;      // レイの原点
        DirectX::SimpleMath::Vector3 direction;  // レイの方向（正規化済み）

        Ray() = default;
        // コンストラクタ：原点と方向を指定
        Ray(const DirectX::SimpleMath::Vector3& o, const DirectX::SimpleMath::Vector3& d)
            : origin(o), direction(d)
        {
            direction.Normalize();
        }

        // バウンディングボックスとの交差判定
         // 成功した場合は outDistance に衝突距離が格納される
        bool Intersects(const DirectX::BoundingBox& box, float& outDistance) const
        {
            return box.Intersects(origin, direction, outDistance);
        }
    };
}
