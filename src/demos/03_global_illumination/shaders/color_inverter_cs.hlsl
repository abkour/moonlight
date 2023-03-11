#define BLOCK_SIZE 8

struct CS_Input
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint GroupIndex         : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct CB_Input
{
    float2 texel_size;
};

Texture2D<float4> src_texture : register(t0);
RWTexture2D<float4> dst_texture : register(u0);
SamplerState LinearClampSampler : register(s0);

ConstantBuffer<CB_Input> CB : register(b0);

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CS_Input IN)
{
    float2 UV = CB.texel_size * (IN.DispatchThreadID.xy);

    float4 texel = src_texture.SampleLevel(LinearClampSampler, UV, 0);
    dst_texture[IN.DispatchThreadID.xy] = float4(float3(1.f, 1.f, 1.f) - texel.xyz, 1.f);
}