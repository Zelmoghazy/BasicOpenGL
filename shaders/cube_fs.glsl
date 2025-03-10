#version 330 core

out vec4 FragColor;

// Passed from the vertex shader
in vec2 TexCoords;
in vec3 ourColor;
in vec3 Normal;
in vec3 FragPos;   

// Uniforms are a way to pass data from our application on the CPU
// to the shaders on the GPU. 
uniform float iTime;
uniform vec2 iResolution;

uniform vec3 viewPos;

struct Material 
{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    vec3      ambient;
    float     shininess;
}; 
  
uniform Material material;

struct Light 
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;


void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // ambient
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

    // diffuse
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos); 
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * vec3(texture(material.diffuse, TexCoords)));

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * vec3(texture(material.specular, TexCoords)));  

    // emmision
    vec3 emission = vec3(texture(material.emission, TexCoords));

    vec3 result = (ambient + diffuse + specular + emission);

    fragColor = vec4(result, 1.0);
}

void main()
{
    vec2 fragCoord = TexCoords * iResolution;
    mainImage(FragColor, fragCoord);
}