#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 UV;
    vec4 Color;
} fs_in;

uniform sampler2D uAlbedo;
uniform vec3 uCameraPos;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform float uSpecStrength;
uniform float uShininess;

void main()
{
    // 1. 빛의 강도 상수 설정 (원하는 밝기에 따라 2.0 ~ 5.0 사이로 조절해보세요)
    const float LIGHT_INTENSITY = 3.0; 
    vec3 effectiveLightColor = uLightColor * LIGHT_INTENSITY;

    vec3 albedo = texture(uAlbedo, fs_in.UV).rgb * fs_in.Color.rgb;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 V = normalize(uCameraPos - fs_in.WorldPos);
    
    // 방향성 조명 방향 고정 (-1, -1, -1) -> 역방향 벡터 L은 (1, 1, 1)
    vec3 L = normalize(vec3(1.0, 1.0, 1.0));
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);

    // Ambient (강도가 너무 낮으면 0.1 정도로 상향 조정 가능)
    vec3 ambient = uAmbientStrength * albedo;

    // Diffuse & Specular에 강도가 적용된 조명 색상 사용
    vec3 diffuse = NdotL * albedo * effectiveLightColor;

    float spec = 0.0;
    if (NdotL > 0.0)
    {
        spec = pow(max(dot(N, H), 0.0), uShininess);
    }
    vec3 specular = uSpecStrength * spec * effectiveLightColor;

    vec3 color = ambient + diffuse + specular;

    // 2. 감마 교정 (표준 2.2 적용)
    // 이 작업만으로도 중간 톤의 밝기가 크게 살아납니다.
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}