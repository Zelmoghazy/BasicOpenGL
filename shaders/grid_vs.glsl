#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

out vec2 uv;
out vec3 fragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() 
{
    fragPos = vec3(model * vec4(aPos, 1.0));
    uv = aUV;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}