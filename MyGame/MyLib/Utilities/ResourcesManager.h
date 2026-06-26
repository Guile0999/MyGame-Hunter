#pragma once
#include <pch.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <Model.h>
#include <Effects.h>

namespace MyLib
{
    class ResourceManager
    {
    public:
        static void Initialize(ID3D11Device* device, const std::wstring& modelDir, const std::wstring& textureDir);
        static void Shutdown();

        // Load model if not cached, return shared_ptr
        static std::shared_ptr<DirectX::Model> GetModel(const std::wstring& filename);

        // TODO: add GetTexture() if you want texture caching too

    private:
        static std::unique_ptr<DirectX::EffectFactory> m_fxFactory;
        static std::unordered_map<std::wstring, std::shared_ptr<DirectX::Model>> m_modelCache;
        static ID3D11Device* m_device;
    };
}