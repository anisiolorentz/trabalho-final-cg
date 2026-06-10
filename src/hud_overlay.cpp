#include "hud_overlay.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale);

static GLuint g_HudProgramID = 0;
static GLuint g_HudVertexArrayObjectID = 0;
static GLuint g_HudVertexBufferObjectID = 0;
static GLint g_HudColorUniform = -1;

static GLuint HudOverlay_LoadShaderFromString(GLenum shader_type, const char* shader_source)
{
    GLuint shader_id = glCreateShader(shader_type);
    glShaderSource(shader_id, 1, &shader_source, NULL);
    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);
    if (compiled_ok != GL_TRUE)
    {
        GLint log_length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<GLchar> log((size_t)log_length + 1);
        glGetShaderInfoLog(shader_id, log_length, &log_length, &log[0]);
        fprintf(stderr, "Erro ao compilar shader do HUD:\n%s\n", &log[0]);
        throw std::runtime_error("Erro ao compilar shader do HUD.");
    }

    return shader_id;
}

void HudOverlay_Init()
{
    const char* vertex_shader =
        "#version 330 core\n"
        "layout (location = 0) in vec2 position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position.xy, 0.0, 1.0);\n"
        "}\n";

    const char* fragment_shader =
        "#version 330 core\n"
        "uniform vec4 hud_color;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "    color = hud_color;\n"
        "}\n";

    GLuint vertex_shader_id = HudOverlay_LoadShaderFromString(GL_VERTEX_SHADER, vertex_shader);
    GLuint fragment_shader_id = HudOverlay_LoadShaderFromString(GL_FRAGMENT_SHADER, fragment_shader);
    g_HudProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);
    g_HudColorUniform = glGetUniformLocation(g_HudProgramID, "hud_color");

    glGenVertexArrays(1, &g_HudVertexArrayObjectID);
    glGenBuffers(1, &g_HudVertexBufferObjectID);

    glBindVertexArray(g_HudVertexArrayObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, g_HudVertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(glm::vec2), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static float HudPixelX(GLFWwindow* window, float pixels)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    (void)height;
    return 2.0f * pixels / (float)width;
}

static float HudPixelY(GLFWwindow* window, float pixels)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    (void)width;
    return 2.0f * pixels / (float)height;
}

static void HudOverlay_DrawVertices(const std::vector<glm::vec2>& vertices, GLenum mode, glm::vec4 color)
{
    if (vertices.empty())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_ALWAYS);

    glUseProgram(g_HudProgramID);
    glUniform4fv(g_HudColorUniform, 1, glm::value_ptr(color));
    glBindVertexArray(g_HudVertexArrayObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, g_HudVertexBufferObjectID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec2), &vertices[0]);
    glDrawArrays(mode, 0, (GLsizei)vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

static void HudOverlay_DrawRect(GLFWwindow* window, float cx, float cy, float width_px, float height_px, glm::vec4 color)
{
    float hw = HudPixelX(window, width_px * 0.5f);
    float hh = HudPixelY(window, height_px * 0.5f);

    std::vector<glm::vec2> vertices;
    vertices.push_back(glm::vec2(cx - hw, cy - hh));
    vertices.push_back(glm::vec2(cx + hw, cy - hh));
    vertices.push_back(glm::vec2(cx + hw, cy + hh));
    vertices.push_back(glm::vec2(cx - hw, cy - hh));
    vertices.push_back(glm::vec2(cx + hw, cy + hh));
    vertices.push_back(glm::vec2(cx - hw, cy + hh));

    HudOverlay_DrawVertices(vertices, GL_TRIANGLES, color);
}

static void HudOverlay_DrawNeutralCircle(GLFWwindow* window)
{
    std::vector<glm::vec2> vertices;
    const int segments = 36;
    float rx = HudPixelX(window, 5.0f);
    float ry = HudPixelY(window, 5.0f);

    for (int i = 0; i < segments; ++i)
    {
        float a = 2.0f * 3.141592f * (float)i / (float)segments;
        vertices.push_back(glm::vec2(cosf(a) * rx, sinf(a) * ry));
    }

    HudOverlay_DrawVertices(vertices, GL_LINE_LOOP, glm::vec4(0.03f, 0.03f, 0.02f, 0.95f));
}

static void HudOverlay_DrawOpenHand(GLFWwindow* window)
{
    glm::vec4 white = glm::vec4(0.96f, 0.96f, 0.93f, 0.96f);
    glm::vec4 shade = glm::vec4(0.68f, 0.72f, 0.78f, 0.82f);
    glm::vec4 black = glm::vec4(0.04f, 0.04f, 0.04f, 0.96f);

    
    
    HudOverlay_DrawRect(window, HudPixelX(window, 2.0f), -HudPixelY(window, 5.0f), 16.0f, 17.0f, black);
    HudOverlay_DrawRect(window, HudPixelX(window, 2.0f), -HudPixelY(window, 5.0f), 12.0f, 13.0f, white);

    for (int i = -2; i <= 1; ++i)
    {
        float x = HudPixelX(window, (float)i * 3.7f + 3.0f);
        float h = (i == -2 || i == 1) ? 17.0f : 21.0f;
        HudOverlay_DrawRect(window, x + HudPixelX(window, 1.0f), HudPixelY(window, 7.0f), 4.8f, h + 3.0f, black);
        HudOverlay_DrawRect(window, x, HudPixelY(window, 8.0f), 2.7f, h, white);
    }

    HudOverlay_DrawRect(window, -HudPixelX(window, 8.0f), -HudPixelY(window, 1.0f), 6.0f, 15.0f, black);
    HudOverlay_DrawRect(window, -HudPixelX(window, 7.0f), -HudPixelY(window, 0.5f), 3.5f, 11.0f, shade);
}

static void HudOverlay_DrawClosedHand(GLFWwindow* window)
{
    glm::vec4 white = glm::vec4(0.96f, 0.96f, 0.93f, 0.96f);
    glm::vec4 shade = glm::vec4(0.68f, 0.72f, 0.78f, 0.86f);
    glm::vec4 black = glm::vec4(0.04f, 0.04f, 0.04f, 0.96f);

    
    
    HudOverlay_DrawRect(window, HudPixelX(window, 1.0f), -HudPixelY(window, 2.0f), 22.0f, 16.0f, black);
    HudOverlay_DrawRect(window, HudPixelX(window, 1.0f), -HudPixelY(window, 2.0f), 17.0f, 12.0f, white);

    for (int i = -2; i <= 1; ++i)
    {
        float x = HudPixelX(window, (float)i * 4.5f + 3.0f);
        HudOverlay_DrawRect(window, x, HudPixelY(window, 7.0f), 4.0f, 7.0f, black);
        HudOverlay_DrawRect(window, x, HudPixelY(window, 7.0f), 2.3f, 4.2f, shade);
    }

    HudOverlay_DrawRect(window, -HudPixelX(window, 8.0f), -HudPixelY(window, 3.0f), 6.0f, 10.0f, black);
    HudOverlay_DrawRect(window, -HudPixelX(window, 7.0f), -HudPixelY(window, 3.0f), 3.5f, 7.0f, white);
}

void HudOverlay_Draw(GLFWwindow* window, HudOverlayState state)
{
    if (state == HUD_HOLDING)
    {
        HudOverlay_DrawClosedHand(window);
        return;
    }

    if (state == HUD_CAN_GRAB)
    {
        HudOverlay_DrawOpenHand(window);
        TextRendering_PrintString(window, "grab", HudPixelX(window, 12.0f), -HudPixelY(window, 22.0f), 0.70f);
        return;
    }

    HudOverlay_DrawNeutralCircle(window);
}

static float HudOverlay_Fract(float value)
{
    return value - floorf(value);
}

static void HudOverlay_DrawStar(GLFWwindow* window, float cx, float cy, float size_px, glm::vec4 color)
{
    HudOverlay_DrawRect(window, cx, cy, size_px, size_px * 0.22f, color);
    HudOverlay_DrawRect(window, cx, cy, size_px * 0.22f, size_px, color);
    HudOverlay_DrawRect(window, cx, cy, size_px * 0.60f, size_px * 0.60f, color);
}

void HudOverlay_DrawVictory(GLFWwindow* window, float elapsed_time)
{
    const glm::vec4 colors[] = {
        glm::vec4(1.00f, 0.95f, 0.28f, 0.95f),
        glm::vec4(0.35f, 0.85f, 1.00f, 0.90f),
        glm::vec4(1.00f, 0.40f, 0.55f, 0.92f),
        glm::vec4(0.72f, 1.00f, 0.45f, 0.90f)
    };

    for (int i = 0; i < 34; ++i)
    {
        float seed = (float)i * 12.9898f;
        float x = -0.82f + 1.64f * HudOverlay_Fract(sinf(seed) * 43758.5453f);
        float drift = sinf(elapsed_time * 2.2f + (float)i) * 0.07f;
        float fall = HudOverlay_Fract(elapsed_time * (0.18f + 0.012f * (float)(i % 7)) + (float)(i % 11) * 0.091f);
        float y = 0.72f - fall * 1.42f;
        float w = 7.0f + (float)(i % 4) * 2.0f;
        float h = 3.0f + (float)(i % 3) * 2.0f;
        HudOverlay_DrawRect(window, x + drift, y, w, h, colors[i % 4]);
    }

    for (int i = 0; i < 10; ++i)
    {
        float a = elapsed_time * 1.8f + (float)i * 0.628318f;
        float radius = 0.20f + 0.018f * (float)(i % 3);
        float x = cosf(a) * radius;
        float y = 0.18f + sinf(a) * radius * 0.65f;
        HudOverlay_DrawStar(window, x, y, 14.0f + (float)(i % 3) * 3.0f, colors[(i + 1) % 4]);
    }

    TextRendering_PrintString(window, "Parabens! Puzzle completo!", -0.43f, 0.05f, 1.25f);

    if (elapsed_time >= 2.0f)
    {
        int remaining = 3 - (int)floorf(elapsed_time - 2.0f);
        if (remaining < 1)
            remaining = 1;
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Recomecando em %d...", remaining);
        TextRendering_PrintString(window, buffer, -0.27f, -0.12f, 0.95f);
    }
}

