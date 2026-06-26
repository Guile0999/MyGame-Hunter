//-------------------------------------------------------------------------------------
// File: FloorPrimitive.h
//
// プリミティブバッチを使用して床を描画するクラス
//
//-------------------------------------------------------------------------------------

#pragma once
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

namespace MyLib
{
    class FloorPrimitive
    {
    private:
        // 静的な頂点データ（1x1の正方形）
        static const DirectX::VertexPositionTexture VERTICES[4];

        // 床サイズ
        float m_Size;

        // 床の位置（ワールド座標）
        DirectX::SimpleMath::Vector3 m_position;

        // 入力レイアウト
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;

        // 共通ステート
        std::unique_ptr<DirectX::CommonStates> m_States;

        // テクスチャ
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_Texture;

        // アルファテスト用エフェクト
        std::unique_ptr<DirectX::AlphaTestEffect> m_Effect;

        // プリミティブバッチ
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>> m_Batch;

    public:
        FloorPrimitive(ID3D11Device* device, float size = 100.0f);
        void Render(
            ID3D11DeviceContext* context,
            const DirectX::SimpleMath::Matrix& view,
            const DirectX::SimpleMath::Matrix& proj);
        void SetPosition(const DirectX::SimpleMath::Vector3& pos);
    };
}