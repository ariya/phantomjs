// Assume we are in SM4+, which has 8 color outputs

void VS_ClearFloat( in float3  inPosition :    POSITION,  in float4  inColor : COLOR,
                   out float4 outPosition : SV_POSITION, out float4 outColor : COLOR)
{
    outPosition = float4(inPosition, 1.0f);
    outColor = inColor;
}

struct PS_OutputFloat
{
    float4 color0 : SV_TARGET0;
    float4 color1 : SV_TARGET1;
    float4 color2 : SV_TARGET2;
    float4 color3 : SV_TARGET3;
#if SM4
    float4 color4 : SV_TARGET4;
    float4 color5 : SV_TARGET5;
    float4 color6 : SV_TARGET6;
    float4 color7 : SV_TARGET7;
#endif
};

PS_OutputFloat PS_ClearFloat(in float4 inPosition : SV_POSITION, in float4 inColor : COLOR)
{
    PS_OutputFloat outColor;
    outColor.color0 = inColor;
    outColor.color1 = inColor;
    outColor.color2 = inColor;
    outColor.color3 = inColor;
#if SM4
    outColor.color4 = inColor;
    outColor.color5 = inColor;
    outColor.color6 = inColor;
    outColor.color7 = inColor;
#endif
    return outColor;
}


void VS_ClearUint( in float3  inPosition :    POSITION,   in uint4  inColor : COLOR,
                   out float4 outPosition : SV_POSITION, out uint4 outColor : COLOR)
{
    outPosition = float4(inPosition, 1.0f);
    outColor = inColor;
}

struct PS_OutputUint
{
    uint4 color0 : SV_TARGET0;
    uint4 color1 : SV_TARGET1;
    uint4 color2 : SV_TARGET2;
    uint4 color3 : SV_TARGET3;
    uint4 color4 : SV_TARGET4;
    uint4 color5 : SV_TARGET5;
    uint4 color6 : SV_TARGET6;
    uint4 color7 : SV_TARGET7;
};

PS_OutputUint PS_ClearUint(in float4 inPosition : SV_POSITION, in uint4 inColor : COLOR)
{
    PS_OutputUint outColor;
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


void VS_ClearSint( in float3  inPosition :    POSITION,   in int4  inColor : COLOR,
                   out float4 outPosition : SV_POSITION, out int4 outColor : COLOR)
{
    outPosition = float4(inPosition, 1.0f);
    outColor = inColor;
}

struct PS_OutputSint
{
    int4 color0 : SV_TARGET0;
    int4 color1 : SV_TARGET1;
    int4 color2 : SV_TARGET2;
    int4 color3 : SV_TARGET3;
    int4 color4 : SV_TARGET4;
    int4 color5 : SV_TARGET5;
    int4 color6 : SV_TARGET6;
    int4 color7 : SV_TARGET7;
};

PS_OutputSint PS_ClearSint(in float4 inPosition : SV_POSITION, in int4 inColor : COLOR)
{
    PS_OutputSint outColor;
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
