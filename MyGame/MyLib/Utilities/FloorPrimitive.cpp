//-------------------------------------------------------------------------------------
// File: FloorPrimitive.cpp
//
// プリミティブによって床を描画する実装
//
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "FloorPrimitive.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

//----------------------------------------
// 静的頂点配列
//----------------------------------------
const VertexPositionTexture MyLib::FloorPrimitive::VERTICES[4] =
{
    { Vector3(1.0f, 0.0f, -1.0f), Vector2(1.0f, 0.0f) },
    { Vector3(1.0f, 0.0f,  1.0f), Vector2(1.0f, 1.0f) },
    { Vector3(-1.0f, 0.0f,  1.0f), Vector2(0.0f, 1.0f) },
    { Vector3(-1.0f, 0.0f, -1.0f), Vector2(0.0f, 0.0f) },
};

//----------------------------------------
// コンストラクタ
//----------------------------------------
MyLib::FloorPrimitive::FloorPrimitive(ID3D11Device* device, float size)
    : m_Size(size)
    , m_position(Vector3::Zero)
{
    // 共通ステート作成
    m_States = std::make_unique<CommonStates>(device);

    // アルファテストエフェクト作成
    m_Effect = std::make_unique<AlphaTestEffect>(device);
    m_Effect->SetAlphaFunction(D3D11_COMPARISON_GREATER);
    m_Effect->SetReferenceAlpha(0);

    // 入力レイアウト作成（Effectに完全一致）
    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexPositionTexture>(
            device,
            m_Effect.get(),
            m_InputLayout.ReleaseAndGetAddressOf()
        )
    );

    // テクスチャ読み込み
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(
            device,
            L"Resources/Textures/ground.png",
            nullptr,
            m_Texture.ReleaseAndGetAddressOf()
        )
    );

    m_Effect->SetTexture(m_Texture.Get());
}

//----------------------------------------
// 描画処理
//----------------------------------------
void MyLib::FloorPrimitive::Render(
    ID3D11DeviceContext* context,
    const Matrix& view,
    const Matrix& proj)
{
    if (!m_Batch)
        m_Batch = std::make_unique<PrimitiveBatch<VertexPositionTexture>>(context);

    // 頂点コピー＆スケール
    VertexPositionTexture verts[4];
    for (int i = 0; i < 4; ++i)
    {
        verts[i] = VERTICES[i];

        XMVECTOR p = XMLoadFloat3(&verts[i].position);
        p *= (m_Size * 0.5f);
        XMStoreFloat3(&verts[i].position, p);
    }

    // 行列設定
    m_Effect->SetWorld(Matrix::CreateTranslation(m_position));
    m_Effect->SetView(view);
    m_Effect->SetProjection(proj);
    m_Effect->Apply(context);

    // ステート設定
    context->IASetInputLayout(m_InputLayout.Get());
    context->OMSetDepthStencilState(m_States->DepthDefault(), 0);
    context->RSSetState(m_States->CullCounterClockwise());

    // 描画
    m_Batch->Begin();
    m_Batch->DrawQuad(verts[0], verts[1], verts[2], verts[3]);
    m_Batch->End();
}

//----------------------------------------
// 位置設定
//----------------------------------------
void MyLib::FloorPrimitive::SetPosition(const Vector3& pos)
{
    m_position = pos;
}