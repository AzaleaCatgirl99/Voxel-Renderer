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
VSOutput VSMain(VSInput input) {
    return {
        mul(SceneParams.mvp, float4(input.position, 1.0)),
        input.color
    };
}

[shader("pixel")]
float4 PSMain(VSOutput vertex) {
    return vertex.color;
}
