#pragma once
#include <pch.h>

namespace MyLib
{
    class Stage
    {
    public:
        Stage();

        void Initialize(
            ID3D11Device* device,
            ID3D11DeviceContext* context,
            DirectX::EffectFactory& fx
        );

        void Render(
            ID3D11DeviceContext* context,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj,
            DirectX::CommonStates& states
        );

        void Update(float elapsedTime);

    private:
        // ===============================
        // モデル
        // ===============================
        std::unique_ptr<DirectX::Model> m_pineTreeModel;
        std::unique_ptr<DirectX::Model> m_skydomeModel;

        // ===============================
		// ワールド行列
        // ===============================
        std::vector<DirectX::SimpleMath::Matrix> m_treeWorlds;
        DirectX::SimpleMath::Matrix              m_skydomeWorld;

        // ===============================
		// 影
       // ===============================
        std::unique_ptr<DirectX::GeometricPrimitive> m_shadowBlob;

		
    };
	    
}