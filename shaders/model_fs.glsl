#version 330 core

out vec4 FragColor;

// Passed from the vertex shader
in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN; // Add this to your vertex shader to pass the TBN matrix

uniform float iTime;
uniform vec2 iResolution;

uniform vec3 viewPos;

struct Material 
{
    float shininess;
    vec3  ambient;
}; 
  
uniform Material material;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;

struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
uniform DirLight dirLight;

vec3 calcNormalFromMap()
{
    vec3 tangentNormal = texture(texture_normal1, TexCoords).xyz * 2.0 - 1.0;
    
    return normalize(TBN * tangentNormal);
}

vec3 CalcDirLight(DirLight light, vec3 norm, vec3 viewDir)
{
    // ambient
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));

    // diffuse
    vec3 lightDir = normalize(-light.direction); 
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * vec3(texture(texture_diffuse1, TexCoords)));

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * vec3(texture(texture_specular1, TexCoords)));  
    
    vec3 result = (ambient + diffuse + specular);

    return result;
}

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
#define NR_POINT_LIGHTS 4  
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir)
{
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                        light.quadratic * (distance * distance)); 

    // ambient
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));
    ambient  *= attenuation; 

    // diffuse
    vec3 lightDir = normalize(light.position - fragPos); 
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * vec3(texture(texture_diffuse1, TexCoords)));
    diffuse  *= attenuation;

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * vec3(texture(texture_specular1, TexCoords)));  
    specular *= attenuation;   

    vec3 result = (ambient + diffuse + specular);

    return result;
}

struct SpotLight 
{
    vec3 position;
    vec3 direction;

    float cutoff;
    float outerCutoff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;

vec3 CalcSpotLight(SpotLight light, vec3 norm, vec3 fragPos, vec3 viewDir)
{
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                        light.quadratic * (distance * distance)); 

    // ambient
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));
    ambient  *= attenuation; 

    // diffuse
    vec3 lightDir = normalize(light.position - fragPos); 
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = light.diffuse * (diff * vec3(texture(texture_diffuse1, TexCoords)));
    diffuse  *= attenuation;

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * vec3(texture(texture_specular1, TexCoords)));  
    specular *= attenuation;   
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon   = (light.cutoff - light.outerCutoff);
    float intensity = smoothstep(0.0, 1.0, (theta - light.outerCutoff) / epsilon);

    diffuse   *= intensity;
    specular  *= intensity;    

    vec3 result = (ambient + diffuse + specular);

    return result;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec3 norm = calcNormalFromMap();
    vec3 viewDir = normalize(viewPos - FragPos);

    // Directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    

    // Spot light
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);    
    
    fragColor = vec4(result, 1.0);
}

void main()
{
    vec2 fragCoord = TexCoords * iResolution;
    mainImage(FragColor, fragCoord);
}