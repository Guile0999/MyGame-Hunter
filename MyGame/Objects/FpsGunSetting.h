#pragma once
#include <pch.h>

namespace MyLib {
    struct FpsGunSetting
    {
        DirectX::SimpleMath::Vector3 offset = { -0.7f, -0.5f, 1.0f }; // right, down, forward adjust model
        DirectX::SimpleMath::Vector3 scale = { 0.0f, 0.0f, 0.0f };
        DirectX::SimpleMath::Vector3 rotation = { -90.0f, -90.0f, 0.0f }; // degrees (a shay sone ka a let yarr like , a lal ka down like

        DirectX::SimpleMath::Vector3 boltOffset = { 0.0f, 0.0f, 0.0f }; // animated offse

    };
}
