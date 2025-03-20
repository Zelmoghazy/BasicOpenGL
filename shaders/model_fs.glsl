#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;

void main()
{    
    vec4 diffuseColor = texture(texture_diffuse1, TexCoords);
    vec4 specularMap = texture(texture_specular1, TexCoords);
    
    // FragColor = diffuseColor;
    
    // FragColor = mix(diffuseColor, specularMap, 0.5); // 20% specular influence
    
    FragColor = texture(texture_diffuse1, TexCoords); // Only diffuse
    // FragColor = texture(texture_specular1, TexCoords); // Only specular
    // FragColor = texture(texture_normal1, TexCoords);   // Only normal map
}