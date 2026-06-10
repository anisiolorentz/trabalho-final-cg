#include "debug_overlay.h"

#include <cstdio>
#include <string>

#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);

void DebugOverlay_ShowEulerAngles(GLFWwindow* window, const DebugOverlayConfig& config)
{
    if (!config.showInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n",
             config.angleZ, config.angleY, config.angleX);

    TextRendering_PrintString(window, buffer, -1.0f + pad/10, -1.0f + 2*pad/10, 1.0f);
}

void DebugOverlay_ShowProjection(GLFWwindow* window, const DebugOverlayConfig& config)
{
    if (!config.showInfoText)
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if (config.usePerspectiveProjection)
        TextRendering_PrintString(window, "Perspective", 1.0f - 13*charwidth, -1.0f + 2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f - 13*charwidth, -1.0f + 2*lineheight/10, 1.0f);
}

void DebugOverlay_ShowFramesPerSecond(GLFWwindow* window, const DebugOverlayConfig& config)
{
    if (!config.showInfoText)
        return;

    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;
    float seconds = (float)glfwGetTime();
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1)*charwidth, 1.0f - lineheight, 1.0f);
}

