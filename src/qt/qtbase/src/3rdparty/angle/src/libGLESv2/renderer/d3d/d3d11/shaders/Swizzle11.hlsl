Texture2D<float4> TextureF2D  : register(t0);
Texture2D<uint4>  TextureUI2D : register(t0);
Texture2D<int4>   TextureI2D  : register(t0);

Texture3D<float4> TextureF3D  : register(t0);
Texture3D<uint4>  TextureUI3D : register(t0);
Texture3D<int4>   TextureI3D  : register(t0);

Texture2DArray<float4> TextureF2DArray  : register(t0);
Texture2DArray<uint4>  TextureUI2DArray : register(t0);
Texture2DArray<int4>   TextureI2DArray  : register(t0);

SamplerState Sampler          : register(s0);

cbuffer SwizzleProperties     : register(b0)
{
    uint4 SwizzleIndices      : packoffset(c0);
}

float4 SwizzleLookup(in float4 sample)
{
    float lookup[6] = { sample[0], sample[1], sample[2], sample[3], 0.0f, 1.0f };
    return float4(lookup[SwizzleIndices[0]], lookup[SwizzleIndices[1]], lookup[SwizzleIndices[2]], lookup[SwizzleIndices[3]]);
}

int4 SwizzleLookup(in int4 sample)
{
    int lookup[6] = { sample[0], sample[1], sample[2], sample[3], 0.0f, 1.0f };
    return int4(lookup[SwizzleIndices[0]], lookup[SwizzleIndices[1]], lookup[SwizzleIndices[2]], lookup[SwizzleIndices[3]]);
}

uint4 SwizzleLookup(in uint4 sample)
{
    uint lookup[6] = { sample[0], sample[1], sample[2], sample[3], 0.0f, 1.0f };
    return uint4(lookup[SwizzleIndices[0]], lookup[SwizzleIndices[1]], lookup[SwizzleIndices[2]], lookup[SwizzleIndices[3]]);
}

float4 PS_SwizzleF2D(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return SwizzleLookup(TextureF2D.Sample(Sampler, inTexCoord));
}

int4 PS_SwizzleI2D(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint2 size;
    TextureI2D.GetDimensions(size.x, size.y);

    return SwizzleLookup(TextureI2D.Load(int3(size * inTexCoord, 0)));
}

uint4 PS_SwizzleUI2D(in float4 inPosition : SV_POSITION, in float2 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint2 size;
    TextureUI2D.GetDimensions(size.x, size.y);

    return SwizzleLookup(TextureUI2D.Load(int3(size * inTexCoord, 0)));
}

float4 PS_SwizzleF3D(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return SwizzleLookup(TextureF3D.Sample(Sampler, inTexCoord));
}

int4 PS_SwizzleI3D(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint3 size;
    TextureI3D.GetDimensions(size.x, size.y, size.z);

    return SwizzleLookup(TextureI3D.Load(int4(size * inTexCoord, 0)));
}

uint4 PS_SwizzleUI3D(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint3 size;
    TextureUI3D.GetDimensions(size.x, size.y, size.z);

    return SwizzleLookup(TextureUI3D.Load(int4(size * inTexCoord, 0)));
}

float4 PS_SwizzleF2DArray(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    return SwizzleLookup(TextureF2DArray.Sample(Sampler, float3(inTexCoord.xy, inLayer)));
}

int4 PS_SwizzleI2DArray(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint3 size;
    TextureI2DArray.GetDimensions(size.x, size.y, size.z);

    return SwizzleLookup(TextureI2DArray.Load(int4(size.xy * inTexCoord.xy, inLayer, 0)));
}

uint4 PS_SwizzleUI2DArray(in float4 inPosition : SV_POSITION, in uint inLayer : SV_RENDERTARGETARRAYINDEX, in float3 inTexCoord : TEXCOORD0) : SV_TARGET0
{
    uint3 size;
    TextureUI2DArray.GetDimensions(size.x, size.y, size.z);

    return SwizzleLookup(TextureUI2DArray.Load(int4(size.xy * inTexCoord.xy, inLayer, 0)));
}
