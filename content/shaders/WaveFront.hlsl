
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


[[vk::binding(0)]]  Texture2D           TextureHDR;
[[vk::binding(1)]]  RWTexture2D<float4> TextureLDR;

float3 Uncharted2Function(float A, float B, float C, float D, float E, float F, float3 x) {
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 ToneMapUncharted2Function(float3 x, float exposure) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    const float W = 11.2;

    float3 numerator = Uncharted2Function(A, B, C, D, E, F, x) * exposure;
    float3 denominator = Uncharted2Function(A, B, C, D, E, F, W);

    return numerator / denominator;

}

float3 LinearToSRGB(float3 color) {
    float3 sRGBLo = color * 12.92;
    const float powExp = 1.0 / 2.2f;
    float3 sRGBHi = (pow(abs(color), float3(powExp, powExp, powExp)) * 1.055) - 0.055;
    float3 sRGB;
    sRGB.x = (color.x <= 0.0031308) ? sRGBLo.x : sRGBHi.x;
    sRGB.y = (color.y <= 0.0031308) ? sRGBLo.y : sRGBHi.y;
    sRGB.z = (color.z <= 0.0031308) ? sRGBLo.z : sRGBHi.z;
    return sRGB;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    float3 colorHDR = TextureHDR.Load(int3(id.xy, 0)).xyz;
    TextureLDR[id.xy] = float4(ToneMapUncharted2Function(colorHDR, 8), 1.0f);
}