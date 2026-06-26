//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}
Texture2D<float4> Texture : register(t0);
sampler Sampler : register(s0);

cbuffer Parameters : register(b0)
{
    float4 DiffuseColor              : packoffset(c0);
    float3 SpecularColor             : packoffset(c1);
    float3 EmissiveColor             : packoffset(c2);
    float  SpecularPower             : packoffset(c2.w);
    
    float3 LightDirection [3]        : packoffset(c3);
    float3 LightDiffuseColor[3]      : packoffset(c6);
    float3 LightSpecularColor[3]     : packoffset(c9);
    
    float3 EyePosition               : packoffset(c12);
    
    float3 FogColor                  : packoffset(c13);
    float4 FogVector                 : packoffset(c14);
    
    float4x4 World                   : packoffset(c15);
    float3x3 WorldInverseTranspose   : packoffset(c19);
    float4x4 WorldViewProj           : packoffset(c22);
};

cbuffer Parameter : register(b1)
{
    float4 torusColor                : packoffset(c0);
  
};

cbuffer Parameters : register(b1)
{
    float Att0 : packoffset(c0.x);
    float Att1 : packoffset(c0.y);
    float Att2 : packoffset(c0.z);
    float3 LightPosition : packoffset(c1);
};



struct PSInputNoFog
{
    float2 TexCoord : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS : TEXCOORD2;
    float4 Diffuse : COLOR0;
};

float4 main(PSInputNoFog pin) : SV_Target0
{
     // ライトの方向を算出する
    float3 lightDirection = pin.PositionWS.xyz - LightPosition;
    
    // ライトまでの距離を算出する
    float lightDistance = length(lightDirection);

     // ライトの距離による光の減衰係数を算出する
    float atten = 1 / (Att0 + Att1 * lightDistance + Att2 * lightDistance * lightDistance);
    

    //
    float4 color = Texture.Sample(Sampler, pin.TexCoord.xy) * pin.Diffuse;
    //法線を正規化
    float3 worldNormal = normalize(pin.NormalWS);

   //光の強さを内積から算出する

    float3 dotL = dot(-LightDirection[0], worldNormal);

   //表面の場合は１、裏面の場合は0
    float3 zeroL = step(0, dotL);
    
    //裏面の場合は黒になる

     float3 diffuse = zeroL * dotL;
    // ライトの距離による減衰を掛ける
    diffuse *= atten;
    


     //マテリアルのディフューズ色を掛ける

     // 光の影響 × マテリアルのディフューズ色 ＋ アンビエント色（エミッシブ色も含む）
    diffuse = diffuse * DiffuseColor.rgb + EmissiveColor;//emissive nay yar mr float3(0.0f,0.0f,.0.0f) nae change lo ya dl
    color.rgb *= diffuse;
    
    //視線vector(表面からカメラのvector)
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);
    
    //half vector
    float3 halfVector = normalize(eyeVector - LightDirection[0]);
    
    //specularの
    float dotH = dot(halfVector, worldNormal);
    
    //
    float3 specular = pow(max(dotH, 0) * zeroL, SpecularPower) * dotL;
    
    //反射されるspecular色を設定
    specular = specular * LightSpecularColor[0] * SpecularColor;
    
   //color.rgb  += specular * color.a * 10;
   //color.rgb  += specular * torusColor.rgb;
     // スペキュラを加える
    color.rgb += specular * color.a;

    
    
   // return torusColor;
    return color;
    //return float4(diffuse,1.0f);
}

