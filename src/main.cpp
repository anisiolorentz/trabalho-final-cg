
















#include <cmath>
#include <cstdio>
#include <cstdlib>


#include <string>
#include <vector>
#include <limits>
#include <algorithm>


#include <glad/glad.h>   
#include <GLFW/glfw3.h>  


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>



#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "hud_overlay.h"
#include "bezier_guide.h"
#include "input_controller.h"
#include "debug_overlay.h"
#include "render_scene.h"

struct GameObject;



void DrawGameObject(const struct GameObject& object); 
BezierGuideRenderConfig CreateBezierGuideConfig(); 
DebugOverlayConfig CreateDebugOverlayConfig(); 
void DrawCeilingLightsAndShadows(); 
void DrawExitBalcony(); 
void DrawCastleDoor(); 
void DrawShadowQuad(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale); 
void DrawShadowEllipse(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale); 
float FindShadowReceiverHeight(const struct GameObject& object, const CollisionAABB& object_box); 
CollisionAABB GetGameObjectCollisionBox(const struct GameObject& object); 
glm::vec3 ResolvePlayerCollisions(glm::vec3 player_position); 
void UpdateDroppedObjectPhysics(); 
void UpdatePlayerVerticalPhysics(); 
bool HorizontalAABBOverlap(const CollisionAABB& a, const CollisionAABB& b); 
float FindPlayerSupportHeight(glm::vec3 player_position); 
float GetWalkableTopHeightForObject(const struct GameObject& object, const CollisionAABB& object_box, glm::vec3 player_position); 
bool HorizontalCircleOverlapsAABB(glm::vec3 center, float radius, const CollisionAABB& box); 
float ComputeHeldObjectScaleFactor(); 
float ComputeHeldObjectDistance(); 
float ComputeHeldObjectMaxScaleRatio(const struct GameObject& object); 
float ComputeHeldObjectMaxDistance(const struct GameObject& object, float preferred_distance); 
float ComputeYawFromDirection(glm::vec3 direction); 
int FindTargetedGameObject(); 
void SelectNextGameObject(); 
void ToggleHeldObject(); 
void CancelHeldObject(); 
void ResetGameState();
void UpdateVictoryState();
void ConfigureInputController(); 
void ReloadShadersFromInput(); 

void TextRendering_Init();



void FramebufferSizeCallback(GLFWwindow* window, int width, int height);






enum ObjectId
{
    SPHERE         = 0,
    BUNNY          = 1,
    PLANE          = 2,
    FLOOR          = 3,
    WALL           = 4,
    TABLE          = 5,
    CUBE_PIECE     = 6,
    TRIANGLE_PIECE = 7,
    CYLINDER_PIECE = 8,
    SELECTED_PIECE = 9,
    CEILING        = 10,
    LIGHT_PANEL    = 11,
    SHADOW         = 12,
    EXIT_MARKER    = 14,
    BEZIER_TRAIL   = 15,
    DIRECTION_ARROW = 16,
    CASTLE_DOOR    = 17,
    BALCONY_CONCRETE = 18
};

struct GameObject
{
    std::string meshName;
    int objectId;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 baseScale;

    glm::vec3 bboxMin;
    glm::vec3 bboxMax;

    bool selectable;
    bool movable;
    bool source;
    bool physicsEnabled;
    bool grounded;
    float verticalVelocity;
};


std::vector<GameObject> g_GameObjects;
int g_SelectedObjectIndex = -1;
int g_HeldObjectIndex = -1;
float g_HeldReferenceScaleFactor = 1.0f;
GameObject g_HeldOriginalObject;
bool g_HeldHasOriginalObject = false;
bool g_HeldWasSourceCopy = false;
float g_HeldYawOffset = 0.0f;



float g_ScreenRatio = 1.0f;


float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

















float g_CameraTheta = 1.0f; 
float g_CameraPhi   = 0.0f; 




const glm::vec4 INITIAL_CAMERA_POSITION = glm::vec4(5.0f, 2.45f, 4.5f, 1.0f);
const float INITIAL_CAMERA_THETA = 1.0f;
const float INITIAL_CAMERA_PHI = 0.0f;
glm::vec4 g_CameraPosition = INITIAL_CAMERA_POSITION;



glm::vec3 g_CameraForward = glm::vec3(0.0f, 0.0f, -1.0f);




float g_CameraSpeed = 5.8f;
float g_PlayerVerticalVelocity = 0.0f;
bool g_PlayerGrounded = true;




const float ROOM_WIDTH  = 30.0f;
const float ROOM_DEPTH  = 18.0f;
const float WALL_HEIGHT = 16.0f;
const float WALL_THICK  = 0.2f;
const float PLAYER_RADIUS = 0.33f;
const float PLAYER_EYE_HEIGHT = 2.45f;
const float PLAYER_JUMP_SPEED = 4.8f;
const float BALCONY_FLOOR_TOP_Y = 11.65f;
const glm::vec3 BALCONY_FLOOR_CENTER = glm::vec3(ROOM_WIDTH / 2.0f - WALL_THICK - 0.77f, BALCONY_FLOOR_TOP_Y - 0.10f, 0.0f);
const glm::vec3 BALCONY_FLOOR_HALF_EXTENTS = glm::vec3(0.77f, 0.10f, 1.93f);
const float BALCONY_SIDE_WALL_HEIGHT = 1.28f;
const glm::vec3 BALCONY_SIDE_WALL_HALF_EXTENTS = glm::vec3(0.77f, BALCONY_SIDE_WALL_HEIGHT / 2.0f, 0.09f);



const glm::vec3 TABLE_POSITION = glm::vec3(-11.0f, 0.0f, -5.5f);
const glm::vec3 TABLE_SCALE    = glm::vec3(0.03f, 0.03f, 0.03f);
const glm::vec3 TABLE_COLLIDER_HALF_EXTENTS = glm::vec3(1.15f, 1.2f, 0.85f);
const float GRAVITY_ACCELERATION = 13.5f;
const float OBJECT_REST_EPSILON = 0.015f;




float g_DeltaTime     = 0.0f;
float g_LastFrameTime = 0.0f;
float g_BezierGuideT  = 0.0f;
float g_ExitAnimTime  = 0.0f;
bool g_GameWon = false;
float g_VictoryTimer = 0.0f;




bool g_KeyW_Pressed = false;
bool g_KeyA_Pressed = false;
bool g_KeyS_Pressed = false;
bool g_KeyD_Pressed = false;



float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;


float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;


bool g_UsePerspectiveProjection = true;


bool g_ShowInfoText = true;


GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_shadow_alpha_uniform;
GLint g_guide_color_uniform;

int main(int argc, char* argv[])
{
    
    
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    
    glfwSetErrorCallback(Input_ErrorCallback);

    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    
    
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    
    
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Seu Cartao - Seu Nome", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    
    
    ConfigureInputController();
    glfwSetKeyCallback(window, Input_KeyCallback);
    
    glfwSetMouseButtonCallback(window, Input_MouseButtonCallback);
    
    glfwSetCursorPosCallback(window, Input_CursorPosCallback);
    
    glfwSetScrollCallback(window, Input_ScrollCallback);

    
    glfwMakeContextCurrent(window);

    
    
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    
    
    
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    
    
    
    
    
    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    FramebufferSizeCallback(window, fb_width, fb_height);

    
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    
    
    
    LoadShadersFromFiles();

    
    LoadTextureImage("../../data/assets/environment/textures/red_brick_diff_1k.jpg");      
    LoadTextureImage("../../data/assets/environment/textures/rocky_terrain_02_diff_1k.jpg"); 
    LoadTextureImage("../../data/assets/environment/textures/laminate_floor_02_diff_1k.png"); 
    LoadTextureImage("../../data/assets/small_wooden_table/textures/small_wooden_table_01_diff_4k.jpg"); 
    LoadTextureImage("../../data/assets/large_castle_door/textures/large_castle_door_diff_4k.jpg"); 
    LoadTextureImage("../../data/assets/environment/textures/concrete_block_wall_03_diff_4k.jpg"); 

    
    ObjModel planemodel("../../data/assets/primitives/plane/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    
    
    
    
    
    ObjModel cubemodel("../../data/assets/primitives/cube/cube.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel spheremodel("../../data/assets/primitives/sphere/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel guidearrowheadmodel("../../data/assets/guide_arrow_head/guide_arrow_head.obj");
    ComputeNormals(&guidearrowheadmodel);
    BuildTrianglesAndAddToVirtualScene(&guidearrowheadmodel);

    ObjModel castledoormodel("../../data/assets/large_castle_door/large_castle_door_4k.obj");
    ComputeNormals(&castledoormodel);
    BuildTrianglesAndAddToVirtualScene(&castledoormodel);

    
    
    
    
    ObjModel tablemodel("../../data/assets/small_wooden_table/small_wooden_table_01_4k.obj");
    ComputeNormals(&tablemodel);
    BuildTrianglesAndAddToVirtualScene(&tablemodel);

    ObjModel puzzlecubemodel("../../data/assets/puzzle_cube/cube.obj");
    ComputeNormals(&puzzlecubemodel);
    BuildTrianglesAndAddToVirtualScene(&puzzlecubemodel);

    ObjModel triangularmodel("../../data/assets/puzzle_ramp/triangular_piece.obj");
    ComputeNormals(&triangularmodel);
    BuildTrianglesAndAddToVirtualScene(&triangularmodel);

    ObjModel cylindermodel("../../data/assets/puzzle_cylinder/cylinder.obj");
    ComputeNormals(&cylindermodel);
    BuildTrianglesAndAddToVirtualScene(&cylindermodel);
    ResetGameState();
    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    
    TextRendering_Init();
    HudOverlay_Init();

    
    
    
    
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    
    g_LastFrameTime = (float)glfwGetTime();

    
    glEnable(GL_DEPTH_TEST);

    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    
    while (!glfwWindowShouldClose(window))
    {
        

        
        
        
        
        
        
        glClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        
        glUseProgram(g_GpuProgramID);

        
        
        
        
        
        
        float now_time  = (float)glfwGetTime();
        g_DeltaTime     = now_time - g_LastFrameTime;
        g_LastFrameTime = now_time;
        g_BezierGuideT += g_DeltaTime * 0.18f;
        if (g_BezierGuideT > 1.0f)
            g_BezierGuideT -= floorf(g_BezierGuideT);
        g_ExitAnimTime += g_DeltaTime;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        glm::vec4 camera_view_vector = glm::vec4(
            -sinf(g_CameraTheta) * cosf(g_CameraPhi),
             sinf(g_CameraPhi),
            -cosf(g_CameraTheta) * cosf(g_CameraPhi),
             0.0f
        );
        glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        
        
        
        
        g_CameraForward = glm::normalize(glm::vec3(
            camera_view_vector.x, camera_view_vector.y, camera_view_vector.z));

        
        
        
        
        
        
        
        
        
        if (g_HeldObjectIndex >= 0)
        {
            GameObject& heldObject = g_GameObjects[g_HeldObjectIndex];
            float scale_factor = ComputeHeldObjectScaleFactor();
            float scale_ratio = scale_factor / std::max(0.01f, g_HeldReferenceScaleFactor);
            scale_ratio = std::min(scale_ratio, ComputeHeldObjectMaxScaleRatio(heldObject));
            heldObject.scale = heldObject.baseScale * scale_ratio;

            float hold_distance = ComputeHeldObjectDistance();
            hold_distance = ComputeHeldObjectMaxDistance(heldObject, hold_distance);

            
            
            glm::vec3 cam_pos3(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
            heldObject.position = cam_pos3 + g_CameraForward * hold_distance;
            if (heldObject.objectId == TRIANGLE_PIECE)
                heldObject.rotation.y = ComputeYawFromDirection(g_CameraForward) + g_HeldYawOffset;

            CollisionAABB held_box = GetGameObjectCollisionBox(heldObject);
            CollisionAABB walkable_room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
            if (held_box.min.y < 0.02f)
            {
                heldObject.position.y += 0.02f - held_box.min.y;
                held_box = GetGameObjectCollisionBox(heldObject);
            }
            if (held_box.max.y > WALL_HEIGHT - 0.25f)
            {
                heldObject.position.y -= held_box.max.y - (WALL_HEIGHT - 0.25f);
                held_box = GetGameObjectCollisionBox(heldObject);
            }

            heldObject.position.x = std::max(walkable_room.min.x + (heldObject.position.x - held_box.min.x),
                                             std::min(walkable_room.max.x - (held_box.max.x - heldObject.position.x),
                                                      heldObject.position.x));
            heldObject.position.z = std::max(walkable_room.min.z + (heldObject.position.z - held_box.min.z),
                                             std::min(walkable_room.max.z - (held_box.max.z - heldObject.position.z),
                                                      heldObject.position.z));
            held_box = GetGameObjectCollisionBox(heldObject);
            CollisionAABB balcony_floor = MakeAABBFromCenterHalfExtents(BALCONY_FLOOR_CENTER, BALCONY_FLOOR_HALF_EXTENTS);
            if (HorizontalAABBOverlap(held_box, balcony_floor) && held_box.max.y >= balcony_floor.max.y)
            {
                float push_left = held_box.max.x - balcony_floor.min.x;
                float push_right = balcony_floor.max.x - held_box.min.x;
                float push_back = held_box.max.z - balcony_floor.min.z;
                float push_front = balcony_floor.max.z - held_box.min.z;
                float best_push = push_left;
                glm::vec3 push = glm::vec3(-push_left - 0.04f, 0.0f, 0.0f);
                if (push_right < best_push && held_box.max.x + push_right + 0.04f <= walkable_room.max.x)
                {
                    best_push = push_right;
                    push = glm::vec3(push_right + 0.04f, 0.0f, 0.0f);
                }
                if (push_back < best_push)
                {
                    best_push = push_back;
                    push = glm::vec3(0.0f, 0.0f, -push_back - 0.04f);
                }
                if (push_front < best_push)
                    push = glm::vec3(0.0f, 0.0f, push_front + 0.04f);
                heldObject.position += push;
            }
        }
        else
        {
            g_SelectedObjectIndex = FindTargetedGameObject();
        }

        
        
        
        
        UpdateDroppedObjectPhysics();
        UpdateVictoryState();
        UpdatePlayerVerticalPhysics();

        
        
        
        
        glm::vec4 horizontal_forward = glm::vec4(camera_view_vector.x, 0.0f, camera_view_vector.z, 0.0f);
        
        float hf_len = sqrtf(horizontal_forward.x*horizontal_forward.x + horizontal_forward.z*horizontal_forward.z);
        if (hf_len > 1e-6f)
            horizontal_forward = horizontal_forward / hf_len;

        
        
        
        
        glm::vec4 right_vector = crossproduct(horizontal_forward, camera_up_vector);
        float r_len = sqrtf(right_vector.x*right_vector.x + right_vector.z*right_vector.z);
        if (r_len > 1e-6f)
            right_vector = right_vector / r_len;

        
        
        
        float step = g_CameraSpeed * g_DeltaTime;
        if (g_KeyW_Pressed) g_CameraPosition += horizontal_forward * step;
        if (g_KeyS_Pressed) g_CameraPosition -= horizontal_forward * step;
        if (g_KeyD_Pressed) g_CameraPosition += right_vector       * step;
        if (g_KeyA_Pressed) g_CameraPosition -= right_vector       * step;

        
        
        
        glm::vec3 camera_pos3(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
        camera_pos3 = ResolvePlayerCollisions(camera_pos3);
        g_CameraPosition.x = camera_pos3.x;
        g_CameraPosition.y = camera_pos3.y;
        g_CameraPosition.z = camera_pos3.z;

        
        g_CameraPosition.w = 1.0f;

        
        
        
        
        glm::mat4 view = Matrix_Camera_View(g_CameraPosition, camera_view_vector, camera_up_vector);

        
        glm::mat4 projection;

        
        
        float nearplane = -0.1f;   
        float farplane  = -100.0f; 
                                   

        if (g_UsePerspectiveProjection)
        {
            
            
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            
            
            
            
            
            
            
            float t = 5.0f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); 

        
        
        
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        const float HALF_WIDTH  = ROOM_WIDTH / 2.0f;
        const float HALF_DEPTH  = ROOM_DEPTH / 2.0f;

        
        
        
        
        model = Matrix_Translate(0.0f, -0.05f, 0.0f)
              * Matrix_Scale(ROOM_WIDTH, 0.1f, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_cube");

        
        
        
        
        
        float wall_center_z = HALF_DEPTH - WALL_THICK / 2.0f;
        float wall_center_x = HALF_WIDTH  - WALL_THICK / 2.0f;

        
        model = Matrix_Translate(0.0f, WALL_HEIGHT / 2.0f, -wall_center_z)
              * Matrix_Scale(ROOM_WIDTH, WALL_HEIGHT, WALL_THICK);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        
        model = Matrix_Translate(0.0f, WALL_HEIGHT / 2.0f, +wall_center_z)
              * Matrix_Scale(ROOM_WIDTH, WALL_HEIGHT, WALL_THICK);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        
        model = Matrix_Translate(-wall_center_x, WALL_HEIGHT / 2.0f, 0.0f)
              * Matrix_Scale(WALL_THICK, WALL_HEIGHT, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        
        model = Matrix_Translate(+wall_center_x, WALL_HEIGHT / 2.0f, 0.0f)
              * Matrix_Scale(WALL_THICK, WALL_HEIGHT, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        
        
        
        
        
        
        
        model = Matrix_Translate(TABLE_POSITION.x, TABLE_POSITION.y, TABLE_POSITION.z)
              * Matrix_Scale(TABLE_SCALE.x, TABLE_SCALE.y, TABLE_SCALE.z);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TABLE);
        DrawVirtualObject("small_wooden_table_01");

        for (size_t i = 0; i < g_GameObjects.size(); ++i)
        {
            DrawGameObject(g_GameObjects[i]);
        }

        DrawExitBalcony();
        DrawCastleDoor();
        DrawBezierGuide(CreateBezierGuideConfig(), DrawVirtualObject);
        DrawCeilingLightsAndShadows();

        
        
        DebugOverlayConfig debug_config = CreateDebugOverlayConfig();
        DebugOverlay_ShowEulerAngles(window, debug_config);

        
        DebugOverlay_ShowProjection(window, debug_config);

        
        
        DebugOverlay_ShowFramesPerSecond(window, debug_config);

        
        
        
        HudOverlayState hud_state = HUD_NEUTRAL;
        if (g_HeldObjectIndex >= 0)
            hud_state = HUD_HOLDING;
        else if (g_SelectedObjectIndex >= 0)
            hud_state = HUD_CAN_GRAB;
        HudOverlay_Draw(window, hud_state);
        if (g_GameWon)
            HudOverlay_DrawVictory(window, g_VictoryTimer);

        
        
        
        
        
        
        glfwSwapBuffers(window);

        
        
        
        
        glfwPollEvents();
    }

    
    glfwTerminate();

    
    return 0;
}



void DrawShadowQuad(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale)
{
    glm::mat4 model = Matrix_Translate(center.x, receiver_y + 0.018f, center.z)
                    * Matrix_Scale(size.x, 0.01f, size.y);

    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, SHADOW);
    glUniform1f(g_shadow_alpha_uniform, alpha_scale);
    DrawVirtualObject("the_cube");
    glUniform1f(g_shadow_alpha_uniform, 0.28f);
}

void DrawShadowEllipse(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale)
{
    glm::mat4 model = Matrix_Translate(center.x, receiver_y + 0.020f, center.z)
                    * Matrix_Scale(size.x, 0.012f, size.y);

    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, SHADOW);
    glUniform1f(g_shadow_alpha_uniform, alpha_scale);
    DrawVirtualObject("the_sphere");
    glUniform1f(g_shadow_alpha_uniform, 0.28f);
}
float FindShadowReceiverHeight(const GameObject& object, const CollisionAABB& object_box)
{
    float receiver_height = 0.0f;

    
    
    
    const float TABLE_VISUAL_TOP = TABLE_POSITION.y + 1.55f;
    CollisionAABB table_shadow_box = MakeAABBFromCenterHalfExtents(
        glm::vec3(TABLE_POSITION.x, TABLE_VISUAL_TOP, TABLE_POSITION.z),
        glm::vec3(TABLE_COLLIDER_HALF_EXTENTS.x, 0.08f, TABLE_COLLIDER_HALF_EXTENTS.z)
    );
    if (HorizontalAABBOverlap(object_box, table_shadow_box) && object_box.min.y >= TABLE_VISUAL_TOP - 0.20f)
        receiver_height = std::max(receiver_height, TABLE_VISUAL_TOP);

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& other = g_GameObjects[i];
        if (&other == &object || other.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB other_box = GetGameObjectCollisionBox(other);
        if (HorizontalAABBOverlap(object_box, other_box) && object_box.min.y >= other_box.max.y - 0.05f)
            receiver_height = std::max(receiver_height, other_box.max.y);
    }

    return receiver_height;
}

void DrawCeilingLightsAndShadows()
{
    
    
    glm::mat4 model = Matrix_Translate(0.0f, WALL_HEIGHT + 0.03f, 0.0f)
                    * Matrix_Scale(ROOM_WIDTH, 0.06f, ROOM_DEPTH);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, CEILING);
    DrawVirtualObject("the_cube");

    const float light_y = WALL_HEIGHT - 0.02f;
    const glm::vec2 light_positions[] = {
        glm::vec2(-12.0f, -7.0f), glm::vec2(-6.0f, -7.0f), glm::vec2( 0.0f, -7.0f), glm::vec2( 6.0f, -7.0f), glm::vec2(12.0f, -7.0f),
        glm::vec2(-10.0f, -3.2f), glm::vec2(-3.3f, -3.2f), glm::vec2( 3.3f, -3.2f), glm::vec2(10.0f, -3.2f),
        glm::vec2(-12.0f,  1.0f), glm::vec2(-6.0f,  1.0f), glm::vec2( 0.0f,  1.0f), glm::vec2( 6.0f,  1.0f), glm::vec2(12.0f,  1.0f),
        glm::vec2(-10.0f,  6.5f), glm::vec2(-3.3f,  6.5f), glm::vec2( 3.3f,  6.5f), glm::vec2(10.0f,  6.5f)
    };

    for (size_t i = 0; i < sizeof(light_positions)/sizeof(light_positions[0]); ++i)
    {
        model = Matrix_Translate(light_positions[i].x, light_y, light_positions[i].y)
              * Matrix_Scale(1.15f, 0.035f, 0.85f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, LIGHT_PANEL);
        DrawVirtualObject("the_cube");
    }

    
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    DrawShadowQuad(glm::vec3(TABLE_POSITION.x, 0.0f, TABLE_POSITION.z),
                   glm::vec2(TABLE_COLLIDER_HALF_EXTENTS.x * 2.15f, TABLE_COLLIDER_HALF_EXTENTS.z * 2.10f),
                   0.0f,
                   0.09f);

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];
        if (object.source)
            continue;

        CollisionAABB box = GetGameObjectCollisionBox(object);
        float receiver_y = FindShadowReceiverHeight(object, box);
        float height_above_receiver = std::max(0.0f, box.min.y - receiver_y);
        if (height_above_receiver < 0.08f)
            continue;

        float softness = std::min(1.40f, 0.15f + height_above_receiver * 0.18f);
        float alpha = std::max(0.04f, 0.24f - height_above_receiver * 0.035f);
        glm::vec2 size((box.max.x - box.min.x) + softness, (box.max.z - box.min.z) + softness);
        if (object.objectId == CYLINDER_PIECE)
            size = glm::vec2((box.max.x - box.min.x) * 0.62f + softness * 0.55f,
                             (box.max.z - box.min.z) * 0.62f + softness * 0.55f);
        glm::vec3 center((box.min.x + box.max.x) * 0.5f, 0.0f, (box.min.z + box.max.z) * 0.5f);

        
        
        
        glm::vec2 from_center(center.x * 0.03f, center.z * 0.03f);
        center.x += from_center.x * height_above_receiver;
        center.z += from_center.y * height_above_receiver;

        if (object.objectId == CYLINDER_PIECE)
            DrawShadowEllipse(center, size, receiver_y, alpha);
        else
            DrawShadowQuad(center, size, receiver_y, alpha);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void DrawGameObject(const GameObject& object)
{
    glm::mat4 model = Matrix_Translate(object.position.x, object.position.y, object.position.z)
                    * Matrix_Rotate_X(object.rotation.x)
                    * Matrix_Rotate_Y(object.rotation.y)
                    * Matrix_Rotate_Z(object.rotation.z)
                    * Matrix_Scale(object.scale.x, object.scale.y, object.scale.z);

    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, object.objectId);
    DrawVirtualObject(object.meshName.c_str());
}

void DrawExitBalcony()
{
    glm::mat4 model = Matrix_Translate(BALCONY_FLOOR_CENTER.x, BALCONY_FLOOR_CENTER.y, BALCONY_FLOOR_CENTER.z)
                    * Matrix_Scale(BALCONY_FLOOR_HALF_EXTENTS.x * 2.0f,
                                   BALCONY_FLOOR_HALF_EXTENTS.y * 2.0f,
                                   BALCONY_FLOOR_HALF_EXTENTS.z * 2.0f);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, BALCONY_CONCRETE);
    DrawVirtualObject("the_cube");

    model = Matrix_Translate(BALCONY_FLOOR_CENTER.x, BALCONY_FLOOR_TOP_Y + 0.006f, BALCONY_FLOOR_CENTER.z)
          * Matrix_Scale(BALCONY_FLOOR_HALF_EXTENTS.x * 2.0f,
                         0.012f,
                         BALCONY_FLOOR_HALF_EXTENTS.z * 2.0f);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, FLOOR);
    DrawVirtualObject("the_cube");

    const float wall_y = BALCONY_FLOOR_TOP_Y + BALCONY_SIDE_WALL_HEIGHT / 2.0f;
    const float wall_z_offset = BALCONY_FLOOR_HALF_EXTENTS.z - BALCONY_SIDE_WALL_HALF_EXTENTS.z;
    for (int side = -1; side <= 1; side += 2)
    {
        model = Matrix_Translate(BALCONY_FLOOR_CENTER.x, wall_y, side * wall_z_offset)
              * Matrix_Scale(BALCONY_SIDE_WALL_HALF_EXTENTS.x * 2.0f,
                             BALCONY_SIDE_WALL_HALF_EXTENTS.y * 2.0f,
                             BALCONY_SIDE_WALL_HALF_EXTENTS.z * 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, BALCONY_CONCRETE);
        DrawVirtualObject("the_cube");
    }
}
void DrawCastleDoor()
{
    const float door_x = ROOM_WIDTH / 2.0f - WALL_THICK - 0.32f;
    const float door_y = BALCONY_FLOOR_TOP_Y + 0.055f;
    const float door_z = 0.0f;
    const float door_scale = 0.0102f;

    glm::mat4 model = Matrix_Translate(door_x, door_y, door_z)
                    * Matrix_Rotate_Y(3.141592f / 2.0f)
                    * Matrix_Scale(door_scale, door_scale, door_scale);

    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, CASTLE_DOOR);
    DrawVirtualObject("large_castle_door_frame");
    DrawVirtualObject("large_castle_door_right");
    DrawVirtualObject("large_castle_door_left");
}
BezierGuideRenderConfig CreateBezierGuideConfig()
{
    BezierGuideRenderConfig config;
    config.roomWidth = ROOM_WIDTH;
    config.wallHeight = WALL_HEIGHT;
    config.wallThickness = WALL_THICK;
    config.tablePosition = TABLE_POSITION;
    config.curveT = g_BezierGuideT;
    config.animationTime = g_ExitAnimTime;
    config.modelUniform = g_model_uniform;
    config.objectIdUniform = g_object_id_uniform;
    config.guideColorUniform = g_guide_color_uniform;
    config.exitMarkerObjectId = EXIT_MARKER;
    config.bezierTrailObjectId = BEZIER_TRAIL;
    config.directionArrowObjectId = DIRECTION_ARROW;
    return config;
}

DebugOverlayConfig CreateDebugOverlayConfig()
{
    DebugOverlayConfig config;
    config.showInfoText = g_ShowInfoText;
    config.usePerspectiveProjection = g_UsePerspectiveProjection;
    config.angleX = g_AngleX;
    config.angleY = g_AngleY;
    config.angleZ = g_AngleZ;
    return config;
}

void ConfigureInputController()
{
    InputControllerContext context;
    context.keyWPressed = &g_KeyW_Pressed;
    context.keyAPressed = &g_KeyA_Pressed;
    context.keySPressed = &g_KeyS_Pressed;
    context.keyDPressed = &g_KeyD_Pressed;
    context.cameraTheta = &g_CameraTheta;
    context.cameraPhi = &g_CameraPhi;
    context.angleX = &g_AngleX;
    context.angleY = &g_AngleY;
    context.angleZ = &g_AngleZ;
    context.playerGrounded = &g_PlayerGrounded;
    context.playerVerticalVelocity = &g_PlayerVerticalVelocity;
    context.playerJumpSpeed = PLAYER_JUMP_SPEED;
    context.usePerspectiveProjection = &g_UsePerspectiveProjection;
    context.showInfoText = &g_ShowInfoText;
    context.toggleHeldObject = ToggleHeldObject;
    context.cancelHeldObject = CancelHeldObject;
    context.selectNextGameObject = SelectNextGameObject;
    context.reloadShaders = ReloadShadersFromInput;
    InputController_SetContext(context);
}

void ReloadShadersFromInput()
{
    LoadShadersFromFiles();
    fprintf(stdout,"Shaders recarregados!\n");
    fflush(stdout);
}
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    
    
    
    
    glViewport(0, 0, width, height);

    
    
    
    
    
    
    
    g_ScreenRatio = (float)width / height;
}

#include "game_logic_impl.inl"








































