#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

struct GLFWwindow;

struct DebugOverlayConfig
{
    bool showInfoText;
    bool usePerspectiveProjection;
    bool useLookAtCamera;
    float angleX;
    float angleY;
    float angleZ;
};

void DebugOverlay_ShowEulerAngles(GLFWwindow* window, const DebugOverlayConfig& config);
void DebugOverlay_ShowProjection(GLFWwindow* window, const DebugOverlayConfig& config);
void DebugOverlay_ShowCameraMode(GLFWwindow* window, const DebugOverlayConfig& config);
void DebugOverlay_ShowFramesPerSecond(GLFWwindow* window, const DebugOverlayConfig& config);

#endif

