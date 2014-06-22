void VS_Clear( in float3  inPosition :    POSITION,  in float4  inColor : COLOR,
              out float4 outPosition : SV_POSITION, out float4 outColor : COLOR)
{
    outPosition = float4(inPosition, 1.0f);
    outColor = inColor;
}

// Assume we are in SM4+, which has 8 color outputs
struct PS_OutputMultiple
{
	float4 color0 : SV_TARGET0;
	float4 color1 : SV_TARGET1;
	float4 color2 : SV_TARGET2;
	float4 color3 : SV_TARGET3;
	float4 color4 : SV_TARGET4;
	float4 color5 : SV_TARGET5;
	float4 color6 : SV_TARGET6;
	float4 color7 : SV_TARGET7;
};

PS_OutputMultiple PS_ClearMultiple(in float4 inPosition : SV_POSITION, in float4 inColor : COLOR)
{
	PS_OutputMultiple outColor;
	outColor.color0 = inColor;
	outColor.color1 = inColor;
	outColor.color2 = inColor;
	outColor.color3 = inColor;
	outColor.color4 = inColor;
	outColor.color5 = inColor;
	outColor.color6 = inColor;
	outColor.color7 = inColor;
	return outColor;
}

float4 PS_ClearSingle(in float4 inPosition : SV_Position, in float4 inColor : COLOR) : SV_Target0
{
	return inColor;
}
