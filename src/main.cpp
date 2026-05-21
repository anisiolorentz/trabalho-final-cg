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
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "hud_overlay.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
void DrawGameObject(const struct GameObject& object); // Desenha uma instância lógica da cena do jogo
void DrawCeilingLightsAndShadows(); // Desenha teto, luminarias e sombras de contato
void DrawShadowQuad(glm::vec3 center, glm::vec2 size, float receiver_y, float alpha_scale); // Sombra plana simplificada
float FindShadowReceiverHeight(const GameObject& object, const CollisionAABB& object_box); // Chao/mesa/peca sob a sombra
CollisionAABB GetGameObjectCollisionBox(const struct GameObject& object); // AABB aproximada de uma peça solta
glm::vec3 ResolvePlayerCollisions(glm::vec3 player_position); // Resolve colisões do jogador com sala, mesa e peças
void UpdateDroppedObjectPhysics(); // Aplica gravidade simplificada nas peças soltas
void UpdatePlayerVerticalPhysics(); // Aplica pulo/gravidade e suporte sobre pecas
bool HorizontalAABBOverlap(const CollisionAABB& a, const CollisionAABB& b); // Sobreposicao XZ entre AABBs
float FindPlayerSupportHeight(glm::vec3 player_position); // Altura do suporte sob o jogador
float GetWalkableTopHeightForObject(const GameObject& object, const CollisionAABB& object_box, glm::vec3 player_position); // Topo caminhavel, incluindo rampa
bool HorizontalCircleOverlapsAABB(glm::vec3 center, float radius, const CollisionAABB& box); // Sobreposicao XZ jogador/AABB
float ComputeHeldObjectScaleFactor(); // Fator de escala conforme câmera/distância
float ComputeHeldObjectDistance(); // Distância da peça segurada à câmera
float ComputeYawFromDirection(glm::vec3 direction); // Alinha objetos ao yaw da camera
int FindTargetedGameObject(); // Retorna a peça selecionável sob a mira central
void SelectNextGameObject(); // Seleciona visualmente a próxima peça manipulável
void ToggleHeldObject(); // Alterna entre pegar e soltar a peça selecionada
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// IDs enviados ao fragment shader via uniform `object_id`. Os valores DEVEM
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
    SHADOW         = 12
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

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;
std::vector<GameObject> g_GameObjects;
int g_SelectedObjectIndex = -1;
int g_HeldObjectIndex = -1;
float g_HeldReferenceScaleFactor = 1.0f;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

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

// Δt entre o frame atual e o anterior. Calculado no início do loop de render
// a partir de glfwGetTime(). Usado para tornar todas as animações e movimentos
// independentes da taxa de quadros (FPS) — requisito do enunciado.
float g_DeltaTime     = 0.0f;
float g_LastFrameTime = 0.0f;

// Estado das teclas de movimento (mantido por KeyCallback). Usamos flags
// (e não eventos GLFW_PRESS isolados) porque o jogador precisa se mover
// continuamente enquanto a tecla estiver pressionada.
bool g_KeyW_Pressed = false;
bool g_KeyA_Pressed = false;
bool g_KeyS_Pressed = false;
bool g_KeyD_Pressed = false;

// Flag que indica se o callback do cursor já recebeu uma posição inicial.
// Como o cursor está capturado (GLFW_CURSOR_DISABLED), o primeiro evento de
// movimento pode reportar um delta enorme — ignoramos esse primeiro evento
// para evitar um "salto" na orientação inicial da câmera.
bool g_CursorInitialized = false;

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

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

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
    glfwSetErrorCallback(ErrorCallback);

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
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

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

    // Inicializa o cronômetro usado para calcular Δt (tempo entre frames).
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
        // (1) Calcular Δt — tempo decorrido desde o frame anterior. Usado para
        //     deixar a velocidade de movimento independente da taxa de quadros.
        float now_time  = (float)glfwGetTime();
        g_DeltaTime     = now_time - g_LastFrameTime;
        g_LastFrameTime = now_time;

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
        //       (caso phi=±π/2 deixe o vetor numericamente quase nulo).
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

        // (5) Integração do movimento WASD usando Δt. Cada tecla pressionada
        //     soma um deslocamento na direção correspondente, multiplicado
        //     pela velocidade da câmera e pelo Δt.
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
        // largura, dimensão compatível com as peças posicionadas em y≈1.6.
        model = Matrix_Translate(TABLE_POSITION.x, TABLE_POSITION.y, TABLE_POSITION.z)
              * Matrix_Scale(TABLE_SCALE.x, TABLE_SCALE.y, TABLE_SCALE.z);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TABLE);
        DrawVirtualObject("small_wooden_table_01");

        for (size_t i = 0; i < g_GameObjects.size(); ++i)
        {
            DrawGameObject(g_GameObjects[i]);
        }

        DrawCeilingLightsAndShadows();

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

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
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
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

CollisionAABB GetGameObjectCollisionBox(const GameObject& object)
{
    // As malhas das pecas foram modeladas com a origem na base (y = 0), nao no
    // centro. Por isso a AABB precisa partir de position.y como "chao" da peca;
    // se tratarmos position como centro, a correcao de gravidade empurra a peca
    // para cima e ela parece flutuar.
    glm::vec3 min = object.position;
    glm::vec3 max = object.position;

    if (object.objectId == CUBE_PIECE)
    {
        min.x += -0.50f * object.scale.x;
        max.x +=  0.50f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  1.00f * object.scale.y;
        min.z += -0.50f * object.scale.z;
        max.z +=  0.50f * object.scale.z;
    }
    else if (object.objectId == TRIANGLE_PIECE)
    {
        min.x += -0.60f * object.scale.x;
        max.x +=  0.60f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  0.80f * object.scale.y;
        min.z += -0.40f * object.scale.z;
        max.z +=  0.40f * object.scale.z;
    }
    else if (object.objectId == CYLINDER_PIECE)
    {
        min.x += -0.50f * object.scale.x;
        max.x +=  0.50f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  0.30f * object.scale.y;
        min.z += -0.50f * object.scale.z;
        max.z +=  0.50f * object.scale.z;
    }
    else
    {
        min += glm::vec3(-0.5f * object.scale.x, 0.0f, -0.5f * object.scale.z);
        max += glm::vec3( 0.5f * object.scale.x, object.scale.y, 0.5f * object.scale.z);
    }

    CollisionAABB box;
    box.min = min;
    box.max = max;
    return box;
}

glm::vec3 ResolvePlayerCollisions(glm::vec3 player_position)
{
    // Primeiro prendemos o jogador dentro da area interna da sala retangular.
    // Depois resolvemos obstaculos internos e prendemos novamente, pois uma
    // colisao perto da parede poderia empurrar o jogador para fora da sala.
    CollisionAABB walkable_room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
    player_position = ClampPointToAABB(player_position, walkable_room);

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
    if (player_feet_y < table_box.max.y - 0.05f)
        player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, table_box);

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];

        // As pecas originais em cima da mesa sao fontes reutilizaveis e ficam
        // cobertas pela colisao maior da mesa. A peca segurada tambem nao deve
        // bloquear o jogador, pois ela acompanha a camera.
        if (object.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB object_box = GetGameObjectCollisionBox(object);
        player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
        if (player_feet_y < object_box.max.y - 0.05f)
            player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, object_box);
    }

    player_position = ClampPointToAABB(player_position, walkable_room);
    return player_position;
}

bool HorizontalCircleOverlapsAABB(glm::vec3 center, float radius, const CollisionAABB& box)
{
    float closest_x = std::max(box.min.x, std::min(box.max.x, center.x));
    float closest_z = std::max(box.min.z, std::min(box.max.z, center.z));
    float dx = center.x - closest_x;
    float dz = center.z - closest_z;
    return dx * dx + dz * dz <= radius * radius;
}

float GetWalkableTopHeightForObject(const GameObject& object, const CollisionAABB& object_box, glm::vec3 player_position)
{
    if (object.objectId != TRIANGLE_PIECE)
        return object_box.max.y;

    // A rampa triangular do OBJ sobe ao longo do eixo X local: em x=-0.6 a
    // altura e 0.8, em x=+0.6 a altura e 0.0. Como atualmente as pecas nao sao
    // rotacionadas pela interacao, usamos esse mapeamento direto para permitir
    // caminhar pela inclinacao em vez de tratar a rampa como um bloco.
    float t = (object_box.max.x - player_position.x) / std::max(0.001f, object_box.max.x - object_box.min.x);
    t = std::max(0.0f, std::min(1.0f, t));
    return object_box.min.y + t * (object_box.max.y - object_box.min.y);
}

float FindPlayerSupportHeight(glm::vec3 player_position)
{
    float support_height = 0.0f;
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    if (HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, table_box)
        && player_feet_y >= table_box.max.y - 0.20f)
    {
        support_height = std::max(support_height, table_box.max.y);
    }

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        if ((int)i == g_HeldObjectIndex)
            continue;

        const GameObject& object = g_GameObjects[i];
        if (object.source)
            continue;

        CollisionAABB object_box = GetGameObjectCollisionBox(object);
        if (!HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, object_box))
            continue;

        float walkable_top = GetWalkableTopHeightForObject(object, object_box, player_position);
        float step_margin = (object.objectId == TRIANGLE_PIECE) ? 0.35f : 0.20f;

        // O jogador pode caminhar sobre a peça quando seus pés estão próximos
        // ou acima da superfície caminhável. Para a rampa, essa superfície é
        // inclinada; para os demais objetos, é o topo da AABB.
        if (player_feet_y >= walkable_top - step_margin)
            support_height = std::max(support_height, walkable_top);
    }

    return support_height;
}

void UpdatePlayerVerticalPhysics()
{
    glm::vec3 player_position(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
    float support_height = FindPlayerSupportHeight(player_position);
    float target_eye_height = support_height + PLAYER_EYE_HEIGHT;

    g_PlayerVerticalVelocity -= GRAVITY_ACCELERATION * g_DeltaTime;
    player_position.y += g_PlayerVerticalVelocity * g_DeltaTime;

    if (player_position.y <= target_eye_height)
    {
        player_position.y = target_eye_height;
        g_PlayerVerticalVelocity = 0.0f;
        g_PlayerGrounded = true;
    }
    else
    {
        g_PlayerGrounded = false;
    }

    g_CameraPosition.y = player_position.y;
}

bool HorizontalAABBOverlap(const CollisionAABB& a, const CollisionAABB& b)
{
    bool separated_x = a.max.x <= b.min.x || a.min.x >= b.max.x;
    bool separated_z = a.max.z <= b.min.z || a.min.z >= b.max.z;
    return !(separated_x || separated_z);
}

float FindSupportHeightForObject(size_t object_index, const CollisionAABB& before_box, const CollisionAABB& after_box)
{
    // O suporte mais básico é o chão da sala. A gravidade nunca deixa a peça
    // passar abaixo desse plano, mantendo a interação estável e previsível.
    float support_height = 0.0f;

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    if (HorizontalAABBOverlap(after_box, table_box)
        && before_box.max.y > table_box.max.y
        && after_box.min.y <= table_box.max.y)
    {
        support_height = std::max(support_height, table_box.max.y);
    }

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        if (i == object_index || (int)i == g_HeldObjectIndex)
            continue;

        const GameObject& other = g_GameObjects[i];
        if (other.source)
            continue;

        CollisionAABB other_box = GetGameObjectCollisionBox(other);
        if (!HorizontalAABBOverlap(after_box, other_box))
            continue;

        // A peca pousa se, durante este frame, sua base cruzou ou entrou no
        // topo de outra peca. Usamos before_box.max.y > top para garantir que
        // estamos tratando um suporte abaixo da peca, e nao algo acima dela.
        if (before_box.max.y > other_box.max.y && after_box.min.y <= other_box.max.y)
            support_height = std::max(support_height, other_box.max.y);
    }

    return support_height;
}

void UpdateDroppedObjectPhysics()
{
    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        GameObject& object = g_GameObjects[i];
        if (!object.physicsEnabled || object.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB before_box = GetGameObjectCollisionBox(object);

        object.verticalVelocity -= GRAVITY_ACCELERATION * g_DeltaTime;
        object.position.y += object.verticalVelocity * g_DeltaTime;

        CollisionAABB after_box = GetGameObjectCollisionBox(object);
        float support_height = FindSupportHeightForObject(i, before_box, after_box);
        if (after_box.min.y <= support_height)
        {
            object.position.y += support_height + OBJECT_REST_EPSILON - after_box.min.y;
            object.verticalVelocity = 0.0f;
            object.grounded = true;
        }
        else
        {
            object.grounded = false;
        }
    }
}

float ComputeHeldObjectScaleFactor()
{
    float vertical_factor = (g_CameraForward.y + 1.0f) / 2.0f;
    vertical_factor = std::max(0.0f, std::min(1.0f, vertical_factor));

    float distance_from_table = glm::length(glm::vec3(
        g_CameraPosition.x - TABLE_POSITION.x,
        0.0f,
        g_CameraPosition.z - TABLE_POSITION.z));
    float distance_factor = std::max(0.0f, std::min(1.0f, (distance_from_table - 4.0f) / 10.0f));

    return 0.10f
         + vertical_factor * vertical_factor * 11.90f
         + vertical_factor * distance_factor * 2.00f;
}

float ComputeHeldObjectDistance()
{
    float vertical_factor = (g_CameraForward.y + 1.0f) / 2.0f;
    vertical_factor = std::max(0.0f, std::min(1.0f, vertical_factor));

    float distance_from_table = glm::length(glm::vec3(
        g_CameraPosition.x - TABLE_POSITION.x,
        0.0f,
        g_CameraPosition.z - TABLE_POSITION.z));
    float distance_factor = std::max(0.0f, std::min(1.0f, (distance_from_table - 4.0f) / 10.0f));

    return 5.75f
         + vertical_factor * vertical_factor * 4.25f
         + vertical_factor * distance_factor * 1.50f;
}

float ComputeYawFromDirection(glm::vec3 direction)
{
    glm::vec3 horizontal(direction.x, 0.0f, direction.z);
    float len = sqrtf(horizontal.x * horizontal.x + horizontal.z * horizontal.z);
    if (len <= 1e-6f)
        return 0.0f;

    horizontal /= len;
    return atan2f(-horizontal.x, -horizontal.z);
}

int FindTargetedGameObject()
{
    glm::vec3 camera_position(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
    int best_index = -1;
    float best_depth = std::numeric_limits<float>::max();

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];
        if (!object.selectable)
            continue;

        float object_radius = std::max(object.scale.x, std::max(object.scale.y, object.scale.z)) * 0.8f + 0.20f;
        float depth = 0.0f;

        // Interseccao da mira central com a peca: aproximamos cada objeto por
        // uma esfera. Isso e suficiente para o puzzle atual e deixa a selecao
        // com proposito claro dentro da logica da aplicacao.
        if (RayIntersectsSphere(camera_position, g_CameraForward, object.position, object_radius, 8.0f, &depth)
            && depth < best_depth)
        {
            best_depth = depth;
            best_index = (int)i;
        }
    }

    return best_index;
}

void SelectNextGameObject()
{
    if (g_GameObjects.empty())
        return;

    if (g_HeldObjectIndex >= 0)
        return;

    int targeted_index = FindTargetedGameObject();
    if (targeted_index >= 0)
    {
        g_SelectedObjectIndex = targeted_index;
        printf("Objeto selecionado pela mira: %s\n", g_GameObjects[targeted_index].meshName.c_str());
        fflush(stdout);
        return;
    }

    int start = g_SelectedObjectIndex;
    for (size_t offset = 1; offset <= g_GameObjects.size(); ++offset)
    {
        int candidate = (start + (int)offset) % (int)g_GameObjects.size();
        if (g_GameObjects[candidate].selectable)
        {
            g_SelectedObjectIndex = candidate;
            printf("Objeto selecionado: %s\n", g_GameObjects[candidate].meshName.c_str());
            fflush(stdout);
            return;
        }
    }
}

void ToggleHeldObject()
{
    if (g_HeldObjectIndex >= 0)
    {
        printf("Objeto solto: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
        fflush(stdout);
        g_GameObjects[g_HeldObjectIndex].selectable = true;
        g_GameObjects[g_HeldObjectIndex].movable = true;
        g_GameObjects[g_HeldObjectIndex].source = false;
        g_GameObjects[g_HeldObjectIndex].physicsEnabled = true;
        g_GameObjects[g_HeldObjectIndex].grounded = false;
        g_GameObjects[g_HeldObjectIndex].verticalVelocity = 0.0f;
        g_HeldObjectIndex = -1;
        g_SelectedObjectIndex = -1;
        return;
    }

    if (g_SelectedObjectIndex < 0)
        g_SelectedObjectIndex = FindTargetedGameObject();

    if (g_SelectedObjectIndex >= 0 && g_GameObjects[g_SelectedObjectIndex].movable)
    {
        if (g_GameObjects[g_SelectedObjectIndex].source)
        {
            // Pecas originais sobre a mesa sao fontes reutilizaveis: pegar uma
            // delas cria uma copia segurada e deixa a fonte no lugar.
            GameObject heldCopy = g_GameObjects[g_SelectedObjectIndex];
            heldCopy.selectable = false;
            heldCopy.source = false;
            heldCopy.physicsEnabled = false;
            heldCopy.grounded = false;
            heldCopy.verticalVelocity = 0.0f;
            heldCopy.baseScale = heldCopy.scale;
            g_GameObjects.push_back(heldCopy);
            g_HeldObjectIndex = (int)g_GameObjects.size() - 1;
        }
        else
        {
            // Pecas ja soltas no mundo podem ser pegas novamente. Nesse caso,
            // seguramos o proprio objeto, sem duplicar a malha logica.
            g_HeldObjectIndex = g_SelectedObjectIndex;
            g_GameObjects[g_HeldObjectIndex].selectable = false;
            g_GameObjects[g_HeldObjectIndex].physicsEnabled = false;
            g_GameObjects[g_HeldObjectIndex].grounded = false;
            g_GameObjects[g_HeldObjectIndex].verticalVelocity = 0.0f;
            g_GameObjects[g_HeldObjectIndex].baseScale = g_GameObjects[g_HeldObjectIndex].scale;
        }

        // Guardamos o fator de escala do exato momento em que o jogador pega
        // a peça. Assim, no primeiro frame segurando, a razão é 1.0 e o objeto
        // aparece com o mesmo tamanho que tinha sobre a mesa/no mundo. A escala
        // só passa a mudar quando o jogador anda ou altera a direção da câmera.
        g_HeldReferenceScaleFactor = ComputeHeldObjectScaleFactor();
        g_SelectedObjectIndex = g_HeldObjectIndex;
        printf("Segurando objeto: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
        fflush(stdout);
    }
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_shadow_alpha_uniform = glGetUniformLocation(g_GpuProgramID, "shadow_alpha");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3); // slot novo no merge — mesa de madeira
    glUniform1f(g_shadow_alpha_uniform, 0.28f);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
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

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
//
// Em modo FPS o cursor está capturado (GLFW_CURSOR_DISABLED), então a posição
// reportada cresce ou diminui livremente — usamos apenas o DELTA entre frames
// para girar a câmera, sem depender de nenhum botão estar pressionado.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Primeira chamada: inicializamos a posição "anterior" do cursor para
    // evitar um salto grande na orientação no primeiro frame.
    if (!g_CursorInitialized)
    {
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        g_CursorInitialized = true;
        return;
    }

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela.
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Sensibilidade do mouse (radianos por pixel). 0.003 dá uma rotação
    // confortável em telas comuns.
    const float sensitivity = 0.003f;

    // Yaw aumenta para a esquerda quando dx > 0? Subtraímos para que mover o
    // mouse para a DIREITA faça a câmera girar para a direita (theta diminui
    // — combinando com a convenção de view_vector = (-sin theta, ..., -cos theta)
    // definida no loop principal).
    g_CameraTheta -= sensitivity * dx;
    // Pitch sobe quando o mouse vai para CIMA (dy < 0 em coordenadas de tela).
    g_CameraPhi   -= sensitivity * dy;

    // Clamp do pitch para evitar gimbal lock: travamos um pouco antes de ±π/2
    // para que o vetor view_vector nunca fique paralelo ao up_vector global
    // (caso em que o produto vetorial em Matrix_Camera_View seria indefinido).
    const float phimax =  3.141592f / 2.0f - 0.01f;
    const float phimin = -phimax;
    if (g_CameraPhi > phimax) g_CameraPhi = phimax;
    if (g_CameraPhi < phimin) g_CameraPhi = phimin;

    // Atualizamos a "última posição" do cursor para o próximo cálculo de delta.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
//
// Na câmera FPS atual, a rodinha não tem função (não há zoom orbital — o
// jogador se aproxima caminhando com WASD). Mantemos o callback registrado
// na GLFW para que seja trivial reaproveitar no futuro (por exemplo, para
// ajustar a velocidade da câmera ou o campo de visão).
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window; (void)xoffset; (void)yoffset;
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // -----------------------------------------------------------------------
    // Movimentação FPS com WASD.
    //
    // Mantemos uma flag booleana por tecla, ligada em GLFW_PRESS e desligada
    // em GLFW_RELEASE (e também em GLFW_REPEAT, embora isso seja redundante).
    // O loop de render usa essas flags + Δt para integrar a posição da câmera.
    // Não tratamos a movimentação aqui, pois GLFW_PRESS dispara apenas uma vez
    // por toque — precisamos do estado contínuo da tecla.
    // -----------------------------------------------------------------------
    if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        bool pressed = (action == GLFW_PRESS);
        if (key == GLFW_KEY_W) g_KeyW_Pressed = pressed;
        if (key == GLFW_KEY_A) g_KeyA_Pressed = pressed;
        if (key == GLFW_KEY_S) g_KeyS_Pressed = pressed;
        if (key == GLFW_KEY_D) g_KeyD_Pressed = pressed;
    }

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Espaço é o pulo do jogador. O impulso só é aplicado quando o jogador
    // está apoiado no chão, mesa ou peça, mantendo uma física simples e estável.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        if (g_PlayerGrounded)
        {
            g_PlayerVerticalVelocity = PLAYER_JUMP_SPEED;
            g_PlayerGrounded = false;
        }
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla E, pegamos ou soltamos a peça selecionada.
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        ToggleHeldObject();
    }

    // Se o usuário apertar a tecla C, alternamos a peça selecionada.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        SelectNextGameObject();
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

