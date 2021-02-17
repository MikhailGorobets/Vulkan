cbuffer ConstantBuffer: register(b0) {
    float4x4 MatrixModel;
    float4x4 MatrixView;
    float4x4 MatrixProjection;
};

void VSMain(uint vertexID : SV_VertexID, out float4 position: SV_Position, [[vk::location(0)]] out float2 texcoord: TEXCOORD0) {
    texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    position = float4(texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f); 
}



float16_t4 PSMain([[vk::location(0)]] float2 texcoord : TEXCOORD0) : SV_Target {
    
    uint16_t lineCount = uint16_t(WaveGetLaneCount());
    uint16_t lineIndex = uint16_t(WaveGetLaneIndex());
   
    float32_t sum = 0.0h;  
    for (uint16_t index = 0; index < 512; index++) {
        sum += 1.0 / lineCount;
    }
  
    return float16_t4(sum, sum, sum, 1.0h);
}

