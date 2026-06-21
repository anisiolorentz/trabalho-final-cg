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
    bool* keyRPressed;

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

    // Estado da câmera look-at: 'useLookAtCamera' indica se o modo de órbita
    // está ativo (o scroll do mouse usa isso para saber se deve dar zoom) e
    // 'lookAtRadius' é o raio da órbita ajustado pela roda do mouse.
    bool* useLookAtCamera;
    float* lookAtRadius;

    InputActionCallback toggleHeldObject;
    InputActionCallback cancelHeldObject;
    InputActionCallback selectNextGameObject;
    InputActionCallback reloadShaders;
    InputActionCallback toggleCameraMode;
};

void InputController_SetContext(const InputControllerContext& context);

void Input_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void Input_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
void Input_ErrorCallback(int error, const char* description);

#endif


