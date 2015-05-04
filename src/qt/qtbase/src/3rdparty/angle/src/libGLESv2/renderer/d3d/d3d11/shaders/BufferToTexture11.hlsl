Buffer<float4>    Buffer4F  : register(t0);
Buffer<int4>      Buffer4I  : register(t0);
Buffer<uint4>     Buffer4UI : register(t0);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    uint index      : TEXCOORD0;
    uint slice      : LAYER;
};

struct GS_OUTPUT
{
    float4 position : SV_Position;
    uint index      : TEXCOORD0;
    uint slice      : SV_RenderTargetArrayIndex;
};

cbuffer BufferCopyParams : register(b0)
{
    uint FirstPixelOffset;
    uint PixelsPerRow;
    uint RowStride;
    uint RowsPerSlice;
    float2 PositionOffset;
    float2 PositionScale;
    int2 TexLocationOffset;
    int2 TexLocationScale;
}

void ComputePositionAndIndex(uint vertexID, out VS_OUTPUT outVertex)
{
    uint PixelsPerSlice = PixelsPerRow * RowsPerSlice;
    uint SliceStride    = RowStride * RowsPerSlice;

    uint slice          = vertexID / PixelsPerSlice;
    uint sliceOffset    = slice * PixelsPerSlice;
    uint row            = (vertexID - sliceOffset) / PixelsPerRow;
    uint col            = vertexID - sliceOffset - (row * PixelsPerRow);

    float2 coords       = float2(float(col), float(row));

    outVertex.position  = float4(PositionOffset + PositionScale * coords, 0.0f, 1.0f);
    outVertex.index     = FirstPixelOffset + slice * SliceStride + row * RowStride + col;
    outVertex.slice     = slice;
}

void VS_BufferToTexture(in uint vertexID : SV_VertexID, out VS_OUTPUT outVertex)
{
    ComputePositionAndIndex(vertexID, outVertex);
}

[maxvertexcount(1)]
void GS_BufferToTexture(point VS_OUTPUT inVertex[1], inout PointStream<GS_OUTPUT> outStream)
{
    GS_OUTPUT outVertex;
    outVertex.position  = inVertex[0].position;
    outVertex.index     = inVertex[0].index;
    outVertex.slice     = inVertex[0].slice;
    outStream.Append(outVertex);
}

float4 PS_BufferToTexture_4F(in float4 inPosition : SV_Position, in uint inIndex : TEXCOORD0) : SV_Target
{
    return Buffer4F.Load(inIndex);
}

int4 PS_BufferToTexture_4I(in float4 inPosition : SV_Position, in uint inIndex : TEXCOORD0) : SV_Target
{
    return Buffer4I.Load(inIndex);
}

uint4 PS_BufferToTexture_4UI(in float4 inPosition : SV_Position, in uint inIndex : TEXCOORD0) : SV_Target
{
    return Buffer4UI.Load(inIndex);
}
