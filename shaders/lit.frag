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

uniform vec3 uLightPos;
uniform vec3 uLightColor;

uniform float uAmbientStrength;
uniform float uSpecStrength;
uniform float uShininess;

void main()
{
    vec3 albedo = texture(uAlbedo, fs_in.UV).rgb * fs_in.Color.rgb;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(uLightPos - fs_in.WorldPos);
    vec3 V = normalize(uCameraPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);

    vec3 ambient = uAmbientStrength * albedo;

    vec3 diffuse = NdotL * albedo * uLightColor;

    float spec = 0.0;
    if (NdotL > 0.0)
    {
        spec = pow(max(dot(N, H), 0.0), uShininess);
    }
    vec3 specular = uSpecStrength * spec * uLightColor;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}
