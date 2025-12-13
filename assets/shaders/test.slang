// test.slang

struct SceneParameters {
    float4x4 mvp;
};

ConstantBuffer<SceneParameters> SceneParams;

struct VSInput {
    float3 position;
    float4 color;
};

struct VSOutput {
    float4 position : SV_Position;
    float4 color;
};

[shader("vertex")]
VSOutput VertexMain(VSInput input) {
    return {
        mul(SceneParams.mvp, float4(input.position, 1.0)),
        input.color
    };
}

[shader("pixel")]
float4 PixelMain(VSOutput vertex) {
    return vertex.color;
}
