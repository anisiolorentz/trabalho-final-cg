#include <glad/glad.h> 


GLuint BuildTriangles(GLsizei &num_indices, GLenum &type_indices)
{
    

    
    
    
    
    
    
    
    
    
    
    
    GLfloat NDC_coefficients[] = {
    
        -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f,
         0.0f,  0.5f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0f
    };

    
    
    
    
    
    
    GLuint VBO_NDC_coefficients_id;
    glGenBuffers(1, &VBO_NDC_coefficients_id);

    
    
    
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);

    
    
    glBindVertexArray(vertex_array_object_id);

    
    
    
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_NDC_coefficients_id);

    
    
    
    
    
    
    
    
    
    
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(NDC_coefficients), NULL, GL_STATIC_DRAW);

    
    
    
    
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(NDC_coefficients), NDC_coefficients);

    
    
    
    
    
    
    
    
    
    
    
    GLuint location = 0; 
    GLint  number_of_dimensions = 4; 
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);

    
    
    
    glEnableVertexAttribArray(location);

    
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    
    
    
    
    
    GLfloat color_coefficients[] = {
    
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 1.0f
    };
    GLuint VBO_color_coefficients_id;
    glGenBuffers(1, &VBO_color_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color_coefficients), color_coefficients);
    location = 1; 
    number_of_dimensions = 4; 
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    
    
    
    
    
    
    
    GLubyte indices[] = { 0,1,2, 2,1,3 }; 

    
    
    
    num_indices = 6;
    type_indices = GL_UNSIGNED_BYTE;

    
    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);

    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);

    
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    
    
    
    
    
    

    
    
    glBindVertexArray(0);

    
    
    return vertex_array_object_id;
}

