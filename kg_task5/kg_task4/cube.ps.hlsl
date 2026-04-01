Texture2D colorTexture : register(t0);
Texture2D normalMapTexture : register(t1);
SamplerState colorSampler : register(s0);

cbuffer GeomBuffer : register(b0) {
    matrix m;
    float4 size;
    float4 color;
    float4 shine;
};

struct Light {
    float4 pos;
    float4 color;
};

cbuffer SceneBuffer : register(b1) {
    matrix vp;
    float4 cameraPos;
    int4 lightCount;
    float4 ambientColor;
    Light lights[10];
};

struct PS_IN {
    float4 pos : SV_POSITION;
    float4 worldPos : POSITION;
    float2 uv : TEXCOORD;
    float3 norm : NORMAL;
    float3 tang : TANGENT;
};

float4 ps(PS_IN pixel) : SV_TARGET {
    float4 texColor = colorTexture.Sample(colorSampler, pixel.uv);
    float3 albedo = texColor.rgb * color.rgb;
    
    float3 finalColor = ambientColor.rgb * albedo;
    
    float3 normal = normalize(pixel.norm);
    float3 tangent = normalize(pixel.tang);
    float3 binorm = normalize(cross(normal, tangent));
    
    float4 normSample = normalMapTexture.Sample(colorSampler, pixel.uv);
    
    if (length(normSample.xyz) > 0.05f) {
        float3 localNorm = normSample.xyz * 2.0f - 1.0f; 
        normal = normalize(localNorm.x * tangent + localNorm.y * binorm + localNorm.z * normal);
    }

    float3 viewDir = normalize(cameraPos.xyz - pixel.worldPos.xyz);

    for (int i = 0; i < lightCount.x; i++) {
        float3 lightDir = lights[i].pos.xyz - pixel.worldPos.xyz;
        float lightDist = length(lightDir);
        lightDir /= lightDist; 
        
        float atten = clamp(1.0f / (lightDist * lightDist), 0.0f, 1.0f);

        float diff = max(dot(normal, lightDir), 0.0f);
        finalColor += albedo * diff * atten * lights[i].color.rgb;

        float3 reflectDir = reflect(-lightDir, normal);
        float spec = 0.0f;
        if (shine.x > 0) {
            spec = pow(max(dot(viewDir, reflectDir), 0.0f), shine.x);
        }
        finalColor += albedo * spec * atten * lights[i].color.rgb;
    }

    return float4(finalColor, texColor.a * color.a);
}