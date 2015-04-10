uniform SamplerState Sampler : register(s0);
uniform Texture2D Texture : register(t0);

void blitvs(in float4 pos0 : TEXCOORD0, in float2 tex0 : TEXCOORD1,
            out float4 gl_Position : SV_POSITION, out float2 coord : TEXCOORD0)
{
    coord = tex0;
    gl_Position = pos0 * float4(1.0, -1.0, 1.0, 1.0);
}

float4 blitps(in float4 gl_Position : SV_POSITION, in float2 coord : TEXCOORD0) : SV_TARGET0
{
    return Texture.Sample(Sampler, coord);
}
