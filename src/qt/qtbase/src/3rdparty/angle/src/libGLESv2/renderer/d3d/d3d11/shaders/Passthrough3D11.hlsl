Texture3D<float4> TextureF  : register(t0);
Texture3D<uint4>  TextureUI : register(t0);
Texture3D<int4>   TextureI  : register(t0);

SamplerState      Sampler   : register(s0);

struct VS_INPUT
{
    float2 Position : POSITION;
    uint   Layer    : LAYER;
    float3 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    uint   Layer    : LAYER;
    float3 TexCoord : TEXCOORD;
};

struct GS_OUTPUT
{
    float4 Position : SV_POSITION;
    uint   Layer    : SV_RENDERTARGETARRAYINDEX;
    float3 TexCoord : TEXCOORD;
};

VS_OUTPUT VS_Passthrough3D(VS_INPUT input)
{
    VS_OUTPUT output;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.Layer = input.Layer;
    output.TexCoord = input.TexCoord;

    return output;
}

[maxvertexcount(3)]
void GS_Passthrough3D(triangle VS_OUTPUT input[3], inout TriangleStream<GS_OUTPUT> outputStream)
{
    GS_OUTPUT output;

    for (int i = 0; i < 3; i++)
    {
        output.Position = input[i].Position;
        output.Layer = input[i].Layer;
        output.TexCoord = input[i].TexCoord;

        outputStream.Append(output);
    }
}

float4 PS_PassthroughRGBA3D(GS_OUTPUT input) : SV_TARGET0
{
    return TextureF.Sample(Sampler, input.TexCoord).rgba;
}

uint4 PS_PassthroughRGBA3DUI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureUI.GetDimensions(size.x, size.y, size.z);

    return TextureUI.Load(int4(size * input.TexCoord, 0)).rgba;
}

int4 PS_PassthroughRGBA3DI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureI.GetDimensions(size.x, size.y, size.z);

    return TextureI.Load(int4(size * input.TexCoord, 0)).rgba;
}

float4 PS_PassthroughRGB3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(TextureF.Sample(Sampler, input.TexCoord).rgb, 1.0f);
}

uint4 PS_PassthroughRGB3DUI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureUI.GetDimensions(size.x, size.y, size.z);

    return uint4(TextureUI.Load(int4(size * input.TexCoord, 0)).rgb, 0);
}

int4 PS_PassthroughRGB3DI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureI.GetDimensions(size.x, size.y, size.z);

    return int4(TextureI.Load(int4(size * input.TexCoord, 0)).rgb, 0);
}

float4 PS_PassthroughRG3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(TextureF.Sample(Sampler, input.TexCoord).rg, 0.0f, 1.0f);
}

uint4 PS_PassthroughRG3DUI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureUI.GetDimensions(size.x, size.y, size.z);

    return uint4(TextureUI.Load(int4(size * input.TexCoord, 0)).rg, 0, 0);
}

int4 PS_PassthroughRG3DI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureI.GetDimensions(size.x, size.y, size.z);

    return int4(TextureI.Load(int4(size * input.TexCoord, 0)).rg, 0, 0);
}

float4 PS_PassthroughR3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(TextureF.Sample(Sampler, input.TexCoord).r, 0.0f, 0.0f, 1.0f);
}

uint4 PS_PassthroughR3DUI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureUI.GetDimensions(size.x, size.y, size.z);

    return uint4(TextureUI.Load(int4(size * input.TexCoord, 0)).r, 0, 0, 0);
}

int4 PS_PassthroughR3DI(GS_OUTPUT input) : SV_TARGET0
{
    uint3 size;
    TextureI.GetDimensions(size.x, size.y, size.z);

    return int4(TextureI.Load(int4(size * input.TexCoord, 0)).r, 0, 0, 0);
}

float4 PS_PassthroughLum3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(TextureF.Sample(Sampler, input.TexCoord).rrr, 1.0f);
}

float4 PS_PassthroughLumAlpha3D(GS_OUTPUT input) : SV_TARGET0
{
    return TextureF.Sample(Sampler, input.TexCoord).rrra;
}
