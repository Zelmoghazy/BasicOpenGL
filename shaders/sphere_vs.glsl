#version 330 core

layout (location = 0) in vec3 aPos;       // Vertex position
layout (location = 1) in vec3 aNormal;    // Vertex normal
layout (location = 2) in vec2 aTexCoord;  // Texture coordinates
// layout (location = 3) in vec3 aColor;     // Vertex color

// Outputs to the fragment shader
out vec2 TexCoord;
out vec3 ourColor;
out vec3 Normal;
out vec3 FragPos;

// Uniforms for transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Transform the vertex position to world space
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    // Pass the fragment position in world space to the fragment shader
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Pass texture coordinates and color to the fragment shader
    // TexCoord = aTexCoord;
    // ourColor = aColor;
    
    // Transform the normal to world space and pass it to the fragment shader
    Normal = mat3(transpose(inverse(model))) * aNormal;
}