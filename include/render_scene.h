#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <tiny_obj_loader.h>

struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true);
};

struct SceneObject
{
    std::string  name;
    size_t       first_index;
    size_t       num_indices;
    GLenum       rendering_mode;
    GLuint       vertex_array_object_id;
    glm::vec3    bbox_min;
    glm::vec3    bbox_max;
};

extern GLuint g_GpuProgramID;
extern GLint g_model_uniform;
extern GLint g_view_uniform;
extern GLint g_projection_uniform;
extern GLint g_object_id_uniform;
extern GLint g_bbox_min_uniform;
extern GLint g_bbox_max_uniform;
extern GLint g_shadow_alpha_uniform;
extern GLint g_guide_color_uniform;

void BuildTrianglesAndAddToVirtualScene(ObjModel* model);
void ComputeNormals(ObjModel* model);
void LoadShadersFromFiles();
void LoadTextureImage(const char* filename);
void DrawVirtualObject(const char* object_name);
void PrintObjModelInfo(ObjModel* model);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

#endif

