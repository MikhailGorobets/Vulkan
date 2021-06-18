
[[vk::binding(1, 4)]] StructuredBuffer<float4> VetrexBuffer;


void VSMain(uint vertexID : SV_VertexID, out float4 position: SV_Position, [[vk::location(0)]] out float2 texcoord: TEXCOORD0) {
    texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    position = VetrexBuffer[vertexID];
}


[[vk::binding(0)]] RWTexture2D<float4> SampleTest0;
[[vk::binding(1)]] RWTexture2D<float4> SampleTest1[2];
[[vk::binding(2)]] RWTexture2D<float4> SampleTest2[];


float4 PSMain([[vk::location(0)]] float2 texcoord : TEXCOORD0) : SV_Target {
    return VetrexBuffer[0] + SampleTest0[uint2(0, 0)] + SampleTest1[0][uint2(0, 0)] + SampleTest2[0][uint2(0, 0)];
}

