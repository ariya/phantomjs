Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

void VS_Passthrough( in float2  inPosition :    POSITION,  in float2  inTexCoord : TEXCOORD0,
                    out float4 outPosition : SV_POSITION, out float2 outTexCoord : TEXCOORD0)
{
    outPosition = float4(inPosition, 0.0f, 1.0f);
    outTexCoord = inTexCoord;
}

float4 PS_PassthroughRGBA(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return Texture.Sample(Sampler, inTexCoord).rgba;
}

float4 PS_PassthroughRGB(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return float4(Texture.Sample(Sampler, inTexCoord).rgb, 1.0f);
}

float4 PS_PassthroughLum(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return float4(Texture.Sample(Sampler, inTexCoord).rrr, 1.0f);
}

float4 PS_PassthroughLumAlpha(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return Texture.Sample(Sampler, inTexCoord).rrra;
}
