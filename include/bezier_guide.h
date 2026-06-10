#ifndef BEZIER_GUIDE_H
#define BEZIER_GUIDE_H

#include <glm/vec3.hpp>

typedef void (*DrawVirtualObjectCallback)(const char* object_name);

struct BezierGuideRenderConfig
{
    float roomWidth;
    float wallHeight;
    float wallThickness;
    glm::vec3 tablePosition;
    float curveT;
    float animationTime;

    int modelUniform;
    int objectIdUniform;
    int guideColorUniform;

    int exitMarkerObjectId;
    int bezierTrailObjectId;
    int directionArrowObjectId;
};

glm::vec3 CubicBezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
void DrawBezierGuide(const BezierGuideRenderConfig& config, DrawVirtualObjectCallback draw_object);

#endif

