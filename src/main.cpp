//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <string>
#include <vector>
#include <limits>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>


// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "hud_overlay.h"
#include "bezier_guide.h"
#include "input_controller.h"
#include "debug_overlay.h"
#include "render_scene.h"

struct GameObject;

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void DrawGameObject(const struct GameObject& object); // Desenha uma instância lógica da cena do jogo
BezierGuideRenderConfig CreateBezierGuideConfig(); // Empacota estado necessario para o guia Bezier
DebugOverlayConfig CreateDebugOverlayConfig(); // Empacota estado para textos de debug
void DrawCeilingLightsAndShadows(); // Desenha teto, luminarias e sombras de contato
void DrawShadowQuad(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale); // Sombra plana simplificada
float FindShadowReceiverHeight(const struct GameObject& object, const CollisionAABB& object_box); // Chao/mesa/peca sob a sombra
CollisionAABB GetGameObjectCollisionBox(const struct GameObject& object); // AABB aproximada de uma peça solta
glm::vec3 ResolvePlayerCollisions(glm::vec3 player_position); // Resolve colisões do jogador com sala, mesa e peças
void UpdateDroppedObjectPhysics(); // Aplica gravidade simplificada nas peças soltas
void UpdatePlayerVerticalPhysics(); // Aplica pulo/gravidade e suporte sobre pecas
bool HorizontalAABBOverlap(const CollisionAABB& a, const CollisionAABB& b); // Sobreposicao XZ entre AABBs
float FindPlayerSupportHeight(glm::vec3 player_position); // Altura do suporte sob o jogador
float GetWalkableTopHeightForObject(const struct GameObject& object, const CollisionAABB& object_box, glm::vec3 player_position); // Topo caminhavel, incluindo rampa
bool HorizontalCircleOverlapsAABB(glm::vec3 center, float radius, const CollisionAABB& box); // Sobreposicao XZ jogador/AABB
float ComputeHeldObjectScaleFactor(); // Fator de escala conforme câmera/distância
float ComputeHeldObjectDistance(); // Distância da peça segurada à câmera
float ComputeYawFromDirection(glm::vec3 direction); // Alinha objetos ao yaw da camera
int FindTargetedGameObject(); // Retorna a peça selecionável sob a mira central
void SelectNextGameObject(); // Seleciona visualmente a próxima peça manipulável
void ToggleHeldObject(); // Alterna entre pegar e soltar a peça selecionada
void ConfigureInputController(); // Conecta estado global aos callbacks de entrada
void ReloadShadersFromInput(); // Recarrega shaders mantendo mensagem de feedback

void TextRendering_Init();

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
// ser idênticos aos #defines em src/shader_fragment.glsl. O merge inseriu
// FLOOR=3 (sala do museu, vinda do HEAD) e deslocou TABLE e as peças do
// puzzle para os próximos índices, mantendo todos os ids do colega.
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
    DIRECTION_ARROW = 16
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

// Abaixo definimos variáveis globais utilizadas em várias funções do código.
std::vector<GameObject> g_GameObjects;
int g_SelectedObjectIndex = -1;
int g_HeldObjectIndex = -1;
float g_HeldReferenceScaleFactor = 1.0f;


// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;


// ----------------------------------------------------------------------------
// Variáveis que definem a câmera em PRIMEIRA PESSOA (FPS).
//
// Substituímos a câmera orbital (look-at em torno da origem) por uma câmera
// livre controlada pelo teclado (WASD) e pelo mouse. Os ângulos g_CameraTheta
// (yaw, rotação em torno do eixo Y global) e g_CameraPhi (pitch, rotação em
// torno do eixo "right" local) agora descrevem a DIREÇÃO DE VISÃO da câmera,
// e não mais a sua posição orbital. A posição efetiva da câmera fica em
// g_CameraPosition (vec4) e é atualizada a cada frame conforme o usuário
// pressiona as teclas WASD.
//
// MERGE: o ramo do colega introduziu g_CameraForward (vec3) usado pela lógica
// de "segurar peça" (held object). Mantemos essa variável e a recalculamos
// todo frame a partir do view_vector da câmera FPS.
// ----------------------------------------------------------------------------
float g_CameraTheta = 1.0f; // Yaw: ângulo no plano XZ (rotação em torno de Y)
float g_CameraPhi   = 0.0f; // Pitch: elevação acima/abaixo do plano XZ

// Posição da câmera (ponto "c" do sistema de coordenadas da câmera). A mesa
// tem tampo em torno de y=1.2; olhos em y=2.4 deixam o jogador com escala mais
// próxima de uma pessoa adulta, cerca do dobro da altura da mesa.
glm::vec4 g_CameraPosition = glm::vec4(5.0f, 2.4f, 4.5f, 1.0f);

// Espelho vec3 do "forward" da câmera, usado pela lógica de peças seguradas
// (DrawGameObject / loop de render). Recomputado todo frame.
glm::vec3 g_CameraForward = glm::vec3(0.0f, 0.0f, -1.0f);

// Velocidade de deslocamento WASD da câmera, em unidades de mundo por segundo.
// Multiplicada por g_DeltaTime no loop de render para garantir velocidade
// independente da taxa de quadros do computador.
float g_CameraSpeed = 5.8f;
float g_PlayerVerticalVelocity = 0.0f;
bool g_PlayerGrounded = true;

// Dimensoes logicas da sala. O render usa os mesmos valores para desenhar
// chao e paredes; o modulo de colisoes usa estes numeros para limitar a area
// caminhavel do jogador.
const float ROOM_WIDTH  = 24.0f;
const float ROOM_DEPTH  = 14.0f;
const float WALL_HEIGHT = 13.5f;
const float WALL_THICK  = 0.2f;
const float PLAYER_RADIUS = 0.35f;
const float PLAYER_EYE_HEIGHT = 2.4f;
const float PLAYER_JUMP_SPEED = 4.8f;

// Posicionamento da mesa no canto da sala. A mesa passa a ser um obstaculo de
// colisao e tambem serve como "bancada" de fontes reutilizaveis das pecas.
const glm::vec3 TABLE_POSITION = glm::vec3(-7.0f, 0.0f, -3.5f);
const glm::vec3 TABLE_SCALE    = glm::vec3(0.03f, 0.03f, 0.03f);
const glm::vec3 TABLE_COLLIDER_HALF_EXTENTS = glm::vec3(1.15f, 1.2f, 0.85f);
const float GRAVITY_ACCELERATION = 13.5f;
const float OBJECT_REST_EPSILON = 0.015f;

// ?t entre o frame atual e o anterior. Calculado no início do loop de render
// a partir de glfwGetTime(). Usado para tornar todas as animações e movimentos
// independentes da taxa de quadros (FPS) — requisito do enunciado.
float g_DeltaTime     = 0.0f;
float g_LastFrameTime = 0.0f;
float g_BezierGuideT  = 0.0f;
float g_ExitAnimTime  = 0.0f;

// Estado das teclas de movimento (mantido por KeyCallback). Usamos flags
// (e não eventos GLFW_PRESS isolados) porque o jogador precisa se mover
// continuamente enquanto a tecla estiver pressionada.
bool g_KeyW_Pressed = false;
bool g_KeyA_Pressed = false;
bool g_KeyS_Pressed = false;
bool g_KeyD_Pressed = false;


// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
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
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(Input_ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Seu Cartao - Seu Nome", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    ConfigureInputController();
    glfwSetKeyCallback(window, Input_KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, Input_MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, Input_CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, Input_ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    // Forçamos a primeira chamada do callback para inicializar g_ScreenRatio e
    // o glViewport. Em telas Retina (macOS), o framebuffer tem dimensões em
    // PIXELS, que podem ser maiores que o tamanho da janela em PONTOS — por
    // isso consultamos glfwGetFramebufferSize() em vez de passar 800x600
    // direto, senão a renderização ocuparia apenas um quarto da janela.
    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    FramebufferSizeCallback(window, fb_width, fb_height);

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");      // TextureImage0
    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1
    LoadTextureImage("../../data/textures/laminate_floor_02_diff_1k.png"); // TextureImage2 — chão do museu (laminate)
    LoadTextureImage("../../data/small_wooden_table_01_4k/textures/small_wooden_table_01_diff_4k.jpg"); // TextureImage3 — mesa de madeira (movida do slot 2 para o 3 no merge)

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    // Cubo unitário usado como bloco de construção da sala do museu (chão e
    // paredes). É instanciado várias vezes com escalas e translações distintas
    // dentro do loop de render — exemplo do requisito "instâncias de objetos".
    // O OBJ define o shape como "the_cube" (não conflita com "puzzle_cube" do
    // ramo do colega, abaixo).
    ObjModel cubemodel("../../data/cube.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel guidearrowheadmodel("../../data/guide_arrow_head.obj");
    ComputeNormals(&guidearrowheadmodel);
    BuildTrianglesAndAddToVirtualScene(&guidearrowheadmodel);

    // Mesa de madeira e peças do puzzle (vindos do ramo do colega). Os shapes
    // dentro destes OBJs têm nomes próprios ("small_wooden_table_01",
    // "puzzle_cube", "puzzle_triangular_piece", "puzzle_cylinder") e portanto
    // convivem com o cubo da sala no mesmo g_VirtualScene.
    ObjModel tablemodel("../../data/small_wooden_table_01_4k/small_wooden_table_01_4k.obj");
    ComputeNormals(&tablemodel);
    BuildTrianglesAndAddToVirtualScene(&tablemodel);

    ObjModel puzzlecubemodel("../../data/puzzle_pieces/cube.obj");
    ComputeNormals(&puzzlecubemodel);
    BuildTrianglesAndAddToVirtualScene(&puzzlecubemodel);

    ObjModel triangularmodel("../../data/puzzle_pieces/triangular_piece.obj");
    ComputeNormals(&triangularmodel);
    BuildTrianglesAndAddToVirtualScene(&triangularmodel);

    ObjModel cylindermodel("../../data/puzzle_pieces/cylinder.obj");
    ComputeNormals(&cylindermodel);
    BuildTrianglesAndAddToVirtualScene(&cylindermodel);

    // Peças interativas iniciais (sobre a mesa). Mantemos a configuração do
    // colega: cada peça é "selecionável", "movível" e marcada como "source"
    // (peça original, antes de ser duplicada para a mão do jogador).
    GameObject cube;
    cube.meshName = "puzzle_cube";
    cube.objectId = CUBE_PIECE;
    cube.position = TABLE_POSITION + glm::vec3(-0.75f, 1.61f, 0.10f);
    cube.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    cube.scale = glm::vec3(0.35f, 0.35f, 0.35f);
    cube.baseScale = cube.scale;
    cube.selectable = true;
    cube.movable = true;
    cube.source = true;
    cube.physicsEnabled = false;
    cube.grounded = true;
    cube.verticalVelocity = 0.0f;
    g_GameObjects.push_back(cube);

    GameObject triangularPiece;
    triangularPiece.meshName = "puzzle_triangular_piece";
    triangularPiece.objectId = TRIANGLE_PIECE;
    triangularPiece.position = TABLE_POSITION + glm::vec3(0.05f, 1.61f, 0.08f);
    triangularPiece.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    triangularPiece.scale = glm::vec3(0.42f, 0.42f, 0.42f);
    triangularPiece.baseScale = triangularPiece.scale;
    triangularPiece.selectable = true;
    triangularPiece.movable = true;
    triangularPiece.source = true;
    triangularPiece.physicsEnabled = false;
    triangularPiece.grounded = true;
    triangularPiece.verticalVelocity = 0.0f;
    g_GameObjects.push_back(triangularPiece);

    GameObject cylinder;
    cylinder.meshName = "puzzle_cylinder";
    cylinder.objectId = CYLINDER_PIECE;
    cylinder.position = TABLE_POSITION + glm::vec3(0.82f, 1.61f, 0.08f);
    cylinder.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    cylinder.scale = glm::vec3(0.45f, 0.45f, 0.45f);
    cylinder.baseScale = cylinder.scale;
    cylinder.selectable = true;
    cylinder.movable = true;
    cylinder.source = true;
    cylinder.physicsEnabled = false;
    cylinder.grounded = true;
    cylinder.verticalVelocity = 0.0f;
    g_GameObjects.push_back(cylinder);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();
    HudOverlay_Init();

    // Capturamos o cursor do mouse dentro da janela e o tornamos invisível.
    // Esse modo (GLFW_CURSOR_DISABLED) é o padrão em jogos FPS: o mouse pode
    // se mover indefinidamente em qualquer direção, sem encostar nas bordas
    // da tela, e o sistema reporta apenas o delta de movimento. Sem isso, a
    // câmera não poderia girar livremente em 360°.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializa o cronômetro usado para calcular ?t (tempo entre frames).
    g_LastFrameTime = (float)glfwGetTime();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // -------------------------------------------------------------------
        // CÂMERA EM PRIMEIRA PESSOA (FPS)
        // -------------------------------------------------------------------
        //
        // (1) Calcular ?t — tempo decorrido desde o frame anterior. Usado para
        //     deixar a velocidade de movimento independente da taxa de quadros.
        float now_time  = (float)glfwGetTime();
        g_DeltaTime     = now_time - g_LastFrameTime;
        g_LastFrameTime = now_time;
        g_BezierGuideT += g_DeltaTime * 0.18f;
        if (g_BezierGuideT > 1.0f)
            g_BezierGuideT -= floorf(g_BezierGuideT);
        g_ExitAnimTime += g_DeltaTime;

        // (2) Reconstruir o vetor de visão a partir dos ângulos yaw (theta)
        //     e pitch (phi), atualizados pelo mouse em CursorPosCallback().
        //
        //     Convenção de eixos: usamos -Z como "frente" inicial (mesma do
        //     template original, em que a câmera começa olhando para a origem
        //     a partir de +Z). Assim, com theta=0 e phi=0, view_vector = (0,0,-1).
        //
        //     Derivação a partir das coordenadas esféricas:
        //         view.x = -sin(theta) * cos(phi)
        //         view.y =  sin(phi)
        //         view.z = -cos(theta) * cos(phi)
        //
        //     Note que NÃO usamos glm::lookAt nem glm::rotate — apenas seno e
        //     cosseno em vetores construídos manualmente.
        glm::vec4 camera_view_vector = glm::vec4(
            -sinf(g_CameraTheta) * cosf(g_CameraPhi),
             sinf(g_CameraPhi),
            -cosf(g_CameraTheta) * cosf(g_CameraPhi),
             0.0f
        );
        glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        // (2.a) Atualiza o espelho vec3 de "forward" usado pela lógica de
        //       peças seguradas (mecânica do colega). camera_view_vector já
        //       é unitário por construção, mas normalizamos por segurança
        //       (caso phi=±p/2 deixe o vetor numericamente quase nulo).
        g_CameraForward = glm::normalize(glm::vec3(
            camera_view_vector.x, camera_view_vector.y, camera_view_vector.z));

        // (2.b) Lógica de "segurar peça" (ramo do colega): se houver uma peça
        //       atualmente segurada, sua posição é fixada bem à frente da
        //       câmera, e sua escala varia com o pitch da câmera — quanto
        //       mais o jogador olha para cima, maior a peça (efeito inspirado
        //       em Superliminal). vertical_factor mapeia a componente Y do
        //       forward da câmera para [0, 1]. Usamos uma curva quadratica
        //       para deixar a alteração de volume mais agressiva quando o
        //       jogador olha para cima, e mantemos o objeto mais longe para a
        //       diferença de proporção ficar visível no espaço da sala.
        if (g_HeldObjectIndex >= 0)
        {
            GameObject& heldObject = g_GameObjects[g_HeldObjectIndex];
            float scale_factor = ComputeHeldObjectScaleFactor();
            float scale_ratio = scale_factor / std::max(0.01f, g_HeldReferenceScaleFactor);
            float hold_distance = ComputeHeldObjectDistance();

            // Convertemos g_CameraPosition (vec4) para vec3 para somar com o
            // forward — a mecânica de hold trabalha em coordenadas afins puras.
            glm::vec3 cam_pos3(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
            heldObject.position = cam_pos3 + g_CameraForward * hold_distance;
            heldObject.scale    = heldObject.baseScale * scale_ratio;
            if (heldObject.objectId == TRIANGLE_PIECE)
            {
                // Ao segurar a rampa, mantemos sua inclinacao apontada para a
                // direcao horizontal da camera. Assim ela sempre aparece a
                // frente do jogador como uma peca pronta para formar caminho.
                heldObject.rotation.y = ComputeYawFromDirection(g_CameraForward) + 3.141592f * 1.5f;
            }

            CollisionAABB held_box = GetGameObjectCollisionBox(heldObject);
            CollisionAABB walkable_room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
            if (held_box.min.y < 0.02f)
                heldObject.position.y += 0.02f - held_box.min.y;

            heldObject.position.x = std::max(walkable_room.min.x + (heldObject.position.x - held_box.min.x),
                                             std::min(walkable_room.max.x - (held_box.max.x - heldObject.position.x),
                                                      heldObject.position.x));
            heldObject.position.z = std::max(walkable_room.min.z + (heldObject.position.z - held_box.min.z),
                                             std::min(walkable_room.max.z - (held_box.max.z - heldObject.position.z),
                                                      heldObject.position.z));
        }
        else
        {
            g_SelectedObjectIndex = FindTargetedGameObject();
        }

        // Gravidade simplificada para peças soltas. Ela roda depois da lógica
        // de peça segurada para que objetos carregados não caiam da "mão" do
        // jogador, mas peças já posicionadas possam repousar no chão, na mesa
        // ou sobre outras peças.
        UpdateDroppedObjectPhysics();
        UpdatePlayerVerticalPhysics();

        // (3) Vetor "horizontal_forward": projeção do view_vector no plano XZ,
        //     usado para o movimento WASD. Forçamos Y=0 para que a câmera não
        //     suba/desça ao apertar W mesmo olhando para cima (estilo FPS
        //     clássico — o jogador anda paralelo ao chão).
        glm::vec4 horizontal_forward = glm::vec4(camera_view_vector.x, 0.0f, camera_view_vector.z, 0.0f);
        // Normalização manual (evitamos glm::normalize por consistência didática).
        float hf_len = sqrtf(horizontal_forward.x*horizontal_forward.x + horizontal_forward.z*horizontal_forward.z);
        if (hf_len > 1e-6f)
            horizontal_forward = horizontal_forward / hf_len;

        // (4) Vetor "right" da câmera: produto vetorial entre forward e up. Em
        //     FPS, "right" também é horizontal (paralelo ao chão), portanto
        //     calculamos a partir do horizontal_forward, não do view_vector.
        //     Usamos crossproduct() definida em include/matrices.h.
        glm::vec4 right_vector = crossproduct(horizontal_forward, camera_up_vector);
        float r_len = sqrtf(right_vector.x*right_vector.x + right_vector.z*right_vector.z);
        if (r_len > 1e-6f)
            right_vector = right_vector / r_len;

        // (5) Integração do movimento WASD usando ?t. Cada tecla pressionada
        //     soma um deslocamento na direção correspondente, multiplicado
        //     pela velocidade da câmera e pelo ?t.
        float step = g_CameraSpeed * g_DeltaTime;
        if (g_KeyW_Pressed) g_CameraPosition += horizontal_forward * step;
        if (g_KeyS_Pressed) g_CameraPosition -= horizontal_forward * step;
        if (g_KeyD_Pressed) g_CameraPosition += right_vector       * step;
        if (g_KeyA_Pressed) g_CameraPosition -= right_vector       * step;

        // Colisao jogador-sala: antes de calcular a matriz de view, tratamos
        // a posicao da camera como ponto do jogador e limitamos sua posicao a
        // uma AABB interna da sala. Assim, WASD nao permite atravessar paredes.
        glm::vec3 camera_pos3(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
        camera_pos3 = ResolvePlayerCollisions(camera_pos3);
        g_CameraPosition.x = camera_pos3.x;
        g_CameraPosition.y = camera_pos3.y;
        g_CameraPosition.z = camera_pos3.z;

        // Mantemos a coordenada w = 1 (ponto) apos a integracao e colisao.
        g_CameraPosition.w = 1.0f;

        // (6) Finalmente montamos a matriz View MANUALMENTE através de
        //     Matrix_Camera_View (include/matrices.h linhas 224–253), que
        //     constrói a base ortonormal {u,v,w} a partir de view e up via
        //     produto vetorial — sem usar glm::lookAt.
        glm::mat4 view = Matrix_Camera_View(g_CameraPosition, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;   // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane" — afastado para
                                   // que toda a sala (20x20 m) caiba no frustum.

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // O zoom ortográfico não é mais controlado por g_CameraDistance
            // (variável removida na refatoração FPS). Usamos um valor fixo —
            // a projeção ortográfica permanece disponível apenas como modo de
            // debug acionável pela tecla O.
            float t = 5.0f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // -------------------------------------------------------------------
        // CENA: sala retangular do museu (chão + 4 paredes).
        //
        // A sala mede ROOM_WIDTH x ROOM_DEPTH no plano XZ e tem WALL_HEIGHT
        // de altura em Y. O cubo unitário carregado de data/cube.obj é
        // INSTANCIADO 5 vezes — uma para o chão (achatado em Y) e uma para
        // cada parede (achatada em X ou Z).
        //
        // Todas as transformações usam APENAS as funções de include/matrices.h
        // (Matrix_Translate, Matrix_Scale, Matrix_Identity), implementadas
        // manualmente. NÃO usamos glm::translate/scale/rotate.
        //
        // MERGE: substituiu o piso 5x5 + 3 paredes em planos do colega (que
        // ficavam logo abaixo na versão original) por esta sala em cubos.
        // A mesa e as peças do puzzle continuam sendo desenhadas mais abaixo,
        // dentro desta sala maior.
        // -------------------------------------------------------------------
        const float HALF_WIDTH  = ROOM_WIDTH / 2.0f;
        const float HALF_DEPTH  = ROOM_DEPTH / 2.0f;

        // --- Chão ---
        // Cubo achatado em Y (0.1 m de espessura) e estendido em X/Z para
        // cobrir toda a sala. Posicionamos com o TOPO em y=0 (centro em
        // y = -0.05) para que o "olho" da câmera (em y = 1.7) fique acima.
        model = Matrix_Translate(0.0f, -0.05f, 0.0f)
              * Matrix_Scale(ROOM_WIDTH, 0.1f, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_cube");

        // --- 4 paredes ---
        // Paredes ao longo do eixo X (face voltada para ±Z): cubos com
        // comprimento ROOM_WIDTH em X, altura WALL_HEIGHT em Y, espessura
        // WALL_THICK em Z. Posicionados em z = ±(HALF_DEPTH - WALL_THICK/2)
        // para que a face interna fique exatamente na borda caminhavel.
        float wall_center_z = HALF_DEPTH - WALL_THICK / 2.0f;
        float wall_center_x = HALF_WIDTH  - WALL_THICK / 2.0f;

        // Parede sul (em z = -HALF)
        model = Matrix_Translate(0.0f, WALL_HEIGHT / 2.0f, -wall_center_z)
              * Matrix_Scale(ROOM_WIDTH, WALL_HEIGHT, WALL_THICK);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        // Parede norte (em z = +HALF)
        model = Matrix_Translate(0.0f, WALL_HEIGHT / 2.0f, +wall_center_z)
              * Matrix_Scale(ROOM_WIDTH, WALL_HEIGHT, WALL_THICK);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        // Parede oeste (em x = -HALF)
        model = Matrix_Translate(-wall_center_x, WALL_HEIGHT / 2.0f, 0.0f)
              * Matrix_Scale(WALL_THICK, WALL_HEIGHT, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        // Parede leste (em x = +HALF)
        model = Matrix_Translate(+wall_center_x, WALL_HEIGHT / 2.0f, 0.0f)
              * Matrix_Scale(WALL_THICK, WALL_HEIGHT, ROOM_DEPTH);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_cube");

        // MERGE: as 3 paredes adicionais em the_plane (parede do fundo +
        // paredes laterais) que existiam aqui foram removidas — agora as 4
        // paredes do museu são desenhadas acima como cubos (sala 20×20).
        //
        // Mesa principal do puzzle. O OBJ exportado veio em escala grande
        // (~50 m), por isso o fator 0.03 — a mesa fica com cerca de 1.5 m de
        // largura, dimensão compatível com as peças posicionadas em y˜1.6.
        model = Matrix_Translate(TABLE_POSITION.x, TABLE_POSITION.y, TABLE_POSITION.z)
              * Matrix_Scale(TABLE_SCALE.x, TABLE_SCALE.y, TABLE_SCALE.z);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TABLE);
        DrawVirtualObject("small_wooden_table_01");

        for (size_t i = 0; i < g_GameObjects.size(); ++i)
        {
            DrawGameObject(g_GameObjects[i]);
        }

        DrawBezierGuide(CreateBezierGuideConfig(), DrawVirtualObject);
        DrawCeilingLightsAndShadows();

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        DebugOverlayConfig debug_config = CreateDebugOverlayConfig();
        DebugOverlay_ShowEulerAngles(window, debug_config);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        DebugOverlay_ShowProjection(window, debug_config);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        DebugOverlay_ShowFramesPerSecond(window, debug_config);

        // Mira fixa e feedback de interacao. O HUD usa o estado calculado pela
        // selecao por raycast: circulo neutro, mao aberta com "grab" sobre uma
        // peca manipulavel, e mao fechada enquanto uma copia esta segurada.
        HudOverlayState hud_state = HUD_NEUTRAL;
        if (g_HeldObjectIndex >= 0)
            hud_state = HUD_HOLDING;
        else if (g_SelectedObjectIndex >= 0)
            hud_state = HUD_CAN_GRAB;
        HudOverlay_Draw(window, hud_state);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura

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

float FindShadowReceiverHeight(const GameObject& object, const CollisionAABB& object_box)
{
    float receiver_height = 0.0f;

    // Para a sombra visual, usamos o tampo aproximado da mesa em vez da AABB de
    // colisao da mesa, que e propositalmente conservadora para bloquear o
    // jogador. Isso evita sombras flutuando acima das pecas da mesa.
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
    // Teto cinza claro, levemente escuro como na referencia: ajuda a vender a
    // ideia de sala interna alta e recebe os painéis luminosos.
    glm::mat4 model = Matrix_Translate(0.0f, WALL_HEIGHT + 0.03f, 0.0f)
                    * Matrix_Scale(ROOM_WIDTH, 0.06f, ROOM_DEPTH);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, CEILING);
    DrawVirtualObject("the_cube");

    const float light_y = WALL_HEIGHT - 0.02f;
    const glm::vec2 light_positions[] = {
        glm::vec2(-9.0f, -5.0f), glm::vec2(-4.8f, -5.0f), glm::vec2( 0.0f, -5.0f), glm::vec2( 4.8f, -5.0f), glm::vec2( 9.0f, -5.0f),
        glm::vec2(-7.0f, -2.0f), glm::vec2(-2.4f, -2.0f), glm::vec2( 2.4f, -2.0f), glm::vec2( 7.0f, -2.0f),
        glm::vec2(-9.0f,  1.2f), glm::vec2(-4.8f,  1.2f), glm::vec2( 0.0f,  1.2f), glm::vec2( 4.8f,  1.2f), glm::vec2( 9.0f,  1.2f),
        glm::vec2(-7.0f,  4.5f), glm::vec2(-2.4f,  4.5f), glm::vec2( 2.4f,  4.5f), glm::vec2( 7.0f,  4.5f)
    };

    for (size_t i = 0; i < sizeof(light_positions)/sizeof(light_positions[0]); ++i)
    {
        model = Matrix_Translate(light_positions[i].x, light_y, light_positions[i].y)
              * Matrix_Scale(1.15f, 0.035f, 0.85f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, LIGHT_PANEL);
        DrawVirtualObject("the_cube");
    }

    // Sombras de contato projetadas verticalmente, coerentes com luminarias no
    // teto. Sao planas e simplificadas, mas dao leitura visual de oclusao sem
    // implementar shadow mapping completo.
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
        glm::vec3 center((box.min.x + box.max.x) * 0.5f, 0.0f, (box.min.z + box.max.z) * 0.5f);

        // Pequeno deslocamento da sombra em relacao ao centro da sala, como se
        // as luminarias distribuidas no teto projetassem sombras levemente
        // inclinadas quando a peca esta alta.
        glm::vec2 from_center(center.x * 0.03f, center.z * 0.03f);
        center.x += from_center.x * height_above_receiver;
        center.z += from_center.y * height_above_receiver;

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
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

#include "game_logic_impl.inl"

