#include "pch.h"
#include "ResourcesManager.h"
using namespace MyLib;
using namespace DirectX;

std::unique_ptr<EffectFactory> ResourceManager::m_fxFactory;
std::unordered_map<std::wstring, std::shared_ptr<Model>> ResourceManager::m_modelCache;
ID3D11Device* ResourceManager::m_device = nullptr;

void ResourceManager::Initialize(ID3D11Device* device, const std::wstring& modelDir, const std::wstring& textureDir)
{
    m_device = device;

    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_fxFactory->SetDirectory(modelDir.c_str());
   
}

void ResourceManager::Shutdown()
{
    m_modelCache.clear();
    m_fxFactory.reset();
    m_device = nullptr;
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::wstring& filename)
{
    auto it = m_modelCache.find(filename);
    if (it != m_modelCache.end())
        return it->second;

    // Load new model
    auto model = Model::CreateFromSDKMESH(m_device, filename.c_str(), *m_fxFactory);
    auto shared = std::shared_ptr<Model>(model.release()); // store in shared_ptr

    m_modelCache[filename] = shared;
    return shared;
}