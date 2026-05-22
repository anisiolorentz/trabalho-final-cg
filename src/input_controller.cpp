#include "input_controller.h"

#include <cstdio>

#include <GLFW/glfw3.h>

void Correcao_KeyCallback(int key, int action, int mod);

static InputControllerContext g_InputContext;
static bool g_LeftMouseButtonPressed = false;
static bool g_RightMouseButtonPressed = false;
static bool g_MiddleMouseButtonPressed = false;
static bool g_CursorInitialized = false;
static double g_LastCursorPosX = 0.0;
static double g_LastCursorPosY = 0.0;

void InputController_SetContext(const InputControllerContext& context)
{
    g_InputContext = context;
}

void Input_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        g_LeftMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_RightMouseButtonPressed = false;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        g_MiddleMouseButtonPressed = false;
}

void Input_CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;

    if (!g_CursorInitialized)
    {
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        g_CursorInitialized = true;
        return;
    }

    float dx = (float)(xpos - g_LastCursorPosX);
    float dy = (float)(ypos - g_LastCursorPosY);
    const float sensitivity = 0.003f;

    *g_InputContext.cameraTheta -= sensitivity * dx;
    *g_InputContext.cameraPhi   -= sensitivity * dy;

    const float phimax =  3.141592f / 2.0f - 0.01f;
    const float phimin = -phimax;
    if (*g_InputContext.cameraPhi > phimax) *g_InputContext.cameraPhi = phimax;
    if (*g_InputContext.cameraPhi < phimin) *g_InputContext.cameraPhi = phimin;

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

void Input_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window; (void)xoffset; (void)yoffset;
}

void Input_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    (void)scancode;

    Correcao_KeyCallback(key, action, mod);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        bool pressed = (action == GLFW_PRESS);
        if (key == GLFW_KEY_W) *g_InputContext.keyWPressed = pressed;
        if (key == GLFW_KEY_A) *g_InputContext.keyAPressed = pressed;
        if (key == GLFW_KEY_S) *g_InputContext.keySPressed = pressed;
        if (key == GLFW_KEY_D) *g_InputContext.keyDPressed = pressed;
    }

    float delta = 3.141592f / 16.0f;
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        *g_InputContext.angleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
        *g_InputContext.angleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        *g_InputContext.angleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && *g_InputContext.playerGrounded)
    {
        *g_InputContext.playerVerticalVelocity = g_InputContext.playerJumpSpeed;
        *g_InputContext.playerGrounded = false;
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        *g_InputContext.usePerspectiveProjection = true;
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        *g_InputContext.usePerspectiveProjection = false;
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        *g_InputContext.showInfoText = !*g_InputContext.showInfoText;
    if (key == GLFW_KEY_E && action == GLFW_PRESS && g_InputContext.toggleHeldObject)
        g_InputContext.toggleHeldObject();
    if (key == GLFW_KEY_C && action == GLFW_PRESS && g_InputContext.selectNextGameObject)
        g_InputContext.selectNextGameObject();
    if (key == GLFW_KEY_R && action == GLFW_PRESS && g_InputContext.reloadShaders)
        g_InputContext.reloadShaders();
}

void Input_ErrorCallback(int error, const char* description)
{
    (void)error;
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}
