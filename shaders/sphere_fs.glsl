#version 330 core

out vec4 FragColor;

// Inputs from the vertex shader
in vec2 TexCoord;
in vec3 ourColor;
in vec3 Normal;
in vec3 FragPos;

// Uniforms for time, resolution, and lighting
uniform float iTime;
uniform vec2 iResolution;

uniform vec3 viewPos;

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

// Light properties
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

// Texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // Ambient lighting
    vec3 ambient = light.ambient * material.ambient;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // Specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    // Combine lighting and object color
    vec3 result = (ambient + diffuse + specular);
    fragColor = vec4(result, 1.0);
}

void main()
{
    // Calculate fragment coordinates and call mainImage
    vec2 fragCoord = TexCoord * iResolution;
    mainImage(FragColor, fragCoord);
}