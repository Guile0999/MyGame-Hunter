#include <pch.h>
#include "Stage.h"
#include <MyLib/Utilities/BinaryFile.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    Stage::Stage() {}
    void Stage::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, EffectFactory& fx)
    {
        // ---------------------------------
		// 松の木のモデルをロード
        // ---------------------------------
        m_pineTreeModel = Model::CreateFromSDKMESH(
            device,
            L"Resources/Models/Resource_PineTree_Group.sdkmesh",
            fx
        );

        // ---------------------------------
		// 松の木の世界が変貌する
        // ---------------------------------
        m_treeWorlds =
        {
         
            // 既存の正面の木
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(10.0f, -1.0f, 30.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(20.0f, -1.0f, 40.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-15.0f, -1.0f, 45.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(25.0f, -1.0f, 60.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-35.0f, -1.0f, 30.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-20.0f, -1.0f, 40.0f),
           
            // 左側の木（X = マイナス方向）
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-40.0f, -1.0f, 15.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-45.0f, -1.0f, 25.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-38.0f, -1.0f, 5.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(-50.0f, -1.0f, 12.0f),
           
            // 右側の木（X = プラス方向）
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(40.0f, -1.0f, 15.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(45.0f, -1.0f, 25.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(38.0f, -1.0f, 5.0f),
            Matrix::CreateScale(10.0f)* Matrix::CreateRotationX(XMConvertToRadians(-90.0f))* Matrix::CreateTranslation(50.0f, -1.0f, 12.0f),
        };

        for (size_t m = 0; m < m_pineTreeModel->meshes.size(); ++m)
        {
            auto& mesh = m_pineTreeModel->meshes[m];
            char buf[128];
            sprintf_s(buf, "Mesh %zu '%ls' has %zu parts\n",
                m, mesh->name.c_str(), mesh->meshParts.size());
            OutputDebugStringA(buf);
        }
        // ---------------------------------
		// スカイドームのモデルをロード
        // ---------------------------------
       
        m_skydomeModel = Model::CreateFromSDKMESH(
            device,
            L"Resources/Models/Skydome.sdkmesh",
            fx
        );
        assert(m_skydomeModel && "Skydome model failed to load");
        // ---------------------------------
        // スカイドームのワールド変換
        // ---------------------------------
        m_skydomeWorld =
            Matrix::CreateScale(900.0f) *
            Matrix::CreateRotationX(XMConvertToRadians(90.0f));


        m_shadowBlob = GeometricPrimitive::CreateCylinder(context, 0.05f, 1.0f);
      
    }

    void Stage::Update(float elapsedTime)
    {
        
    }

    void Stage::Render(
        ID3D11DeviceContext* context,
        const Matrix& view,
        const Matrix& proj,
        CommonStates& states
    )
    {
	
		// カメラの位置を取得
        Matrix invView = view.Invert();
        Vector3 camPos = invView.Translation();
        // ---------------------------------
		// スカイドームの描画
        // ---------------------------------
        if (m_skydomeModel)
        {
            context->OMSetDepthStencilState(states.DepthNone(), 0);
            context->RSSetState(states.CullNone());

            Matrix skyWorld =
                Matrix::CreateScale(900.0f) *
                Matrix::CreateRotationX(XMConvertToRadians(90.0f)) *
                Matrix::CreateTranslation(0.0f, -50.0f, 0.0f);

            m_skydomeModel->Draw(
                context,
                states,
                skyWorld,
                view,
                proj
            );

            context->OMSetDepthStencilState(nullptr, 0);
            context->RSSetState(nullptr);
        }

		// 松の木の描画
        int treeIndex = 0;
        for (const auto& world : m_treeWorlds)
        {
            m_pineTreeModel->Draw(context, states, world, view, proj);
        }

		// 影の描画
        for (const auto& world : m_treeWorlds)
        {
            Vector3 treePos = world.Translation();
            Matrix shadowWorld =
                Matrix::CreateScale(8.0f, 1.0f, 8.0f) *
                Matrix::CreateTranslation(treePos.x, -0.99f, treePos.z);
            m_shadowBlob->Draw(shadowWorld, view, proj,
                Color(0.0f, 0.0f, 0.0f, 0.4f));
        }


    }
}