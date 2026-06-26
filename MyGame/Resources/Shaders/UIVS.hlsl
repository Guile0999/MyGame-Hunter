#include "UI.hlsli"

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;

	output.pos = float4(input.pos,1);

	output.color = input.color;
	output.tex = input.tex;
	return output;
}