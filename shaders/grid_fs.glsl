#version 330 core

in vec2 uv;
in vec3 fragPos;
out vec4 FragColor;
uniform vec3 cameraPos;

void main() 
{
    // Grid parameters
    float gridSize = 0.5;
    float lineWidth = 0.005;
    
    // Calculate grid lines
    vec2 coord = fragPos.xz / gridSize;
    vec2 grid = abs(fract(coord - 0.5) - 0.5);
    float line = min(grid.x, grid.y);
    
    // Fade with distance
    float distance = length(fragPos - cameraPos);
    float fade = 1.0 - smoothstep(10.0, 50.0, distance);
    
    // Main grid lines
    float alpha = (1.0 - smoothstep(lineWidth - 0.003, lineWidth + 0.003, line)) * fade * 0.5;
    vec3 color = vec3(0.5);
    
    // Discard fully transparent fragments
    if(alpha < 0.01) discard;
    
    FragColor = vec4(color, alpha);
}