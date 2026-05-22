#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

struct GLFWwindow;

typedef void (*InputActionCallback)();

struct InputControllerContext
{
    bool* keyWPressed;
    bool* keyAPressed;
    bool* keySPressed;
    bool* keyDPressed;

    float* cameraTheta;
    float* cameraPhi;

    float* angleX;
    float* angleY;
    float* angleZ;

    bool* playerGrounded;
    float* playerVerticalVelocity;
    float playerJumpSpeed;

    bool* usePerspectiveProjection;
    bool* showInfoText;

    InputActionCallback toggleHeldObject;
    InputActionCallback selectNextGameObject;
    InputActionCallback reloadShaders;
};

void InputController_SetContext(const InputControllerContext& context);

void Input_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void Input_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
void Input_ErrorCallback(int error, const char* description);

#endif
