struct ImGuiParam {
    float2 Scale;
    float2 Translate;
};

struct VertexShaderInput {
    [[vk::location(0)]] float32_t2 Position : POSITION;
    [[vk::location(1)]] float32_t2 Texcoord : TEXCOORD;
    [[vk::location(2)]] float16_t4 Color    : COLOR;
};

struct PixelShaderInput {
    [[vk::location(0)]] float32_t2 Texcoord : TEXCOORD;
    [[vk::location(1)]] float16_t4 Color    : COLOR;
};

[[vk::push_constant]] ConstantBuffer<ImGuiParam> PushConstants;

PixelShaderInput VSMain(VertexShaderInput input, out float4 position : SV_Position) {
    PixelShaderInput output;
    output.Texcoord = input.Texcoord;
    output.Color = input.Color;
    position = float4(input.Position * PushConstants.Scale + PushConstants.Translate, 0.0f, 1.0f);
    return output;
}

[[vk::binding(0, 0)]] Texture2D<float4> SampledTexture;
[[vk::binding(1, 0)]] SamplerState      SamplerLinear;

float16_t3 sRGBToLinear(float16_t3 color) {
    color = max(6.10352e-5h, color);
    return color > 0.04045h ? pow(color * (1.0h / 1.055h) + 0.0521327h, 2.4h) : color * (1.0h / 12.92h);
}


float16_t4 PSMain(PixelShaderInput input) : SV_Target0 {
    return float16_t4(sRGBToLinear(input.Color.xyz), input.Color.a) * float16_t4(SampledTexture.Sample(SamplerLinear, input.Texcoord));
}

