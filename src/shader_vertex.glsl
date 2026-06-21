#version 330 core



layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;





out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

// Normal no espaço-DO-OBJETO (sem aplicar a Model matrix). É usada no fragment
// shader para a amostragem triplanar das peças do puzzle: como tanto a normal
// quanto a posição ficam em coordenadas locais da malha, a textura permanece
// "colada" na peça mesmo quando ela é transladada ou girada (tecla R).
out vec4 normal_model;

void main()
{
    
    
    
    
    
    
    
    
    
    
    

    gl_Position = projection * view * model * model_coefficients;

    
    
    
    
    
    
    
    
    
    

    
    

    
    position_world = model * model_coefficients;

    
    position_model = model_coefficients;

    
    
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;


    texcoords = texture_coefficients;

    // Normal crua da malha (espaço do objeto), repassada para o fragment shader.
    normal_model = normal_coefficients;
    normal_model.w = 0.0;
}


