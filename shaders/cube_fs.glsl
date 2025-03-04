#version 330 core

out vec4 FragColor;

// Passed from the vertex shader
in vec2 TexCoord;
in vec3 ourColor;
in vec3 Normal;
in vec3 FragPos;   

// Uniforms are a way to pass data from our application on the CPU
// to the shaders on the GPU. 
uniform float iTime;
uniform vec2 iResolution;

uniform vec3 viewPos;

uniform sampler2D texture1;
uniform sampler2D texture2;

struct Material 
{
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
    float shininess;
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
    vec3 ambient = light.ambient * material.ambient;

    // diffuse
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos); 
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * material.diffuse);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  

    vec3 result = (ambient + diffuse + specular);
    // fragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), sin(iTime))+ vec4(result, 1.0);
    // fragColor = vec4(result * lightColor, 1.0);
    fragColor = vec4(result, 1.0);
}

void main()
{
    vec2 fragCoord = TexCoord * iResolution;
    mainImage(FragColor, fragCoord);
}