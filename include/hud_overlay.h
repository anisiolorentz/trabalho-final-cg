#ifndef HUD_OVERLAY_H
#define HUD_OVERLAY_H

struct GLFWwindow;

enum HudOverlayState
{
    HUD_NEUTRAL = 0,
    HUD_CAN_GRAB = 1,
    HUD_HOLDING = 2
};

void HudOverlay_Init();
void HudOverlay_Draw(GLFWwindow* window, HudOverlayState state);

#endif
