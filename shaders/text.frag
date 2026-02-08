#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uFontAtlas;
uniform vec4 uColor; // RGBA

void main()
{
    // Atlas is single-channel and swizzled so alpha comes from RED.
    float a = texture(uFontAtlas, vUV).a;
    float outA = a * uColor.a;

    if (outA < 0.01)
    {
        discard;
    }

    FragColor = vec4(uColor.rgb, outA);
}
