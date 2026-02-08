#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV1;
layout(location = 3) in vec2 aUV2;
layout(location = 4) in vec2 aUV3;
layout(location = 5) in vec2 aUV4;
layout(location = 6) in vec4 aColor;
layout(location = 7) in vec3 aTangent;
layout(location = 8) in vec3 aBitangent;
layout(location = 9) in uvec4 aBoneIndex;
layout(location = 10) in vec4 aBoneWeight;


out VS_OUT
{
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 UV;
    vec4 Color;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vec4 world = uModel * vec4(aPos, 1.0);
    vs_out.WorldPos = world.xyz;

    mat3 nrmMat = mat3(transpose(inverse(uModel)));
    vs_out.WorldNormal = normalize(nrmMat * aNormal);

    vs_out.UV = aUV1;
    vs_out.Color = aColor;

    gl_Position = uProj * uView * world;
}
