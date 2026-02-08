#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec4 aColor;

out vec4 vColor;

uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vColor = aColor;
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
