#include "bezier_guide.h"

#include <algorithm>
#include <cmath>

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>

static glm::mat4 GuideTranslate(float tx, float ty, float tz)
{
    glm::mat4 M(1.0f);
    M[3][0] = tx;
    M[3][1] = ty;
    M[3][2] = tz;
    return M;
}

static glm::mat4 GuideScale(float sx, float sy, float sz)
{
    glm::mat4 M(1.0f);
    M[0][0] = sx;
    M[1][1] = sy;
    M[2][2] = sz;
    return M;
}

static glm::mat4 GuideRotateY(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    glm::mat4 M(1.0f);
    M[0][0] = c;
    M[0][2] = -s;
    M[2][0] = s;
    M[2][2] = c;
    return M;
}

static glm::mat4 GuideRotateZ(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    glm::mat4 M(1.0f);
    M[0][0] = c;
    M[0][1] = s;
    M[1][0] = -s;
    M[1][1] = c;
    return M;
}

glm::vec3 CubicBezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    float u = 1.0f - t;
    return (u*u*u) * p0
         + (3.0f*u*u*t) * p1
         + (3.0f*u*t*t) * p2
         + (t*t*t) * p3;
}

static void DrawGuideCube(const BezierGuideRenderConfig& config, glm::mat4 model, DrawVirtualObjectCallback draw_object)
{
    glUniformMatrix4fv(config.modelUniform, 1, GL_FALSE, glm::value_ptr(model));
    draw_object("the_cube");
}

static void DrawGuideObject(const BezierGuideRenderConfig& config, glm::mat4 model, const char* object_name, DrawVirtualObjectCallback draw_object)
{
    glUniformMatrix4fv(config.modelUniform, 1, GL_FALSE, glm::value_ptr(model));
    draw_object(object_name);
}

static glm::mat4 GuideBasisFromDirection(glm::vec3 position, glm::vec3 direction)
{
    glm::vec3 safe_direction = glm::normalize(direction);
    float yaw = atan2f(-safe_direction.z, safe_direction.x);
    float pitch = asinf(std::max(-1.0f, std::min(1.0f, safe_direction.y)));
    return GuideTranslate(position.x, position.y, position.z)
         * GuideRotateY(yaw)
         * GuideRotateZ(pitch);
}

static void DrawGuideSphere(const BezierGuideRenderConfig& config, glm::mat4 model, float intensity, DrawVirtualObjectCallback draw_object)
{
    glUniform3f(config.guideColorUniform, intensity, intensity, intensity);
    DrawGuideObject(config, model, "the_sphere", draw_object);
}

void DrawBezierGuide(const BezierGuideRenderConfig& config, DrawVirtualObjectCallback draw_object)
{
    const float max_pulse = 1.08f;
    const float exit_x = config.roomWidth / 2.0f - config.wallThickness - 0.62f;
    const float exit_z = 0.0f;
    const float exit_y = config.wallHeight - (1.36f * max_pulse) - 0.10f;

    glm::vec3 p0 = config.tablePosition + glm::vec3(-0.85f, 2.35f, 0.10f);
    glm::vec3 p1 = config.tablePosition + glm::vec3(1.4f, 3.55f, 2.7f);
    glm::vec3 p2 = glm::vec3(exit_x - 3.3f, config.wallHeight - 0.80f, 0.8f);
    glm::vec3 p3 = glm::vec3(exit_x - 0.95f, config.wallHeight - 0.65f, exit_z);

    float arrow_t = config.curveT;
    glm::vec3 arrow_position = CubicBezier(arrow_t, p0, p1, p2, p3);
    glm::vec3 next_position = CubicBezier(std::min(1.0f, arrow_t + 0.025f), p0, p1, p2, p3);
    if (arrow_t > 0.975f)
        next_position = arrow_position + (arrow_position - CubicBezier(arrow_t - 0.025f, p0, p1, p2, p3));
    glm::vec3 arrow_direction = glm::normalize(next_position - arrow_position);

    const int trail_samples = 26;
    const float trail_length = 0.56f;
    for (int i = 0; i < trail_samples; ++i)
    {
        float age = (float)i / (float)(trail_samples - 1);
        float t = arrow_t - trail_length * age;
        if (t < 0.0f)
            continue;

        glm::vec3 trail_position = CubicBezier(t, p0, p1, p2, p3);
        glm::vec3 trail_next = CubicBezier(std::min(1.0f, t + 0.035f), p0, p1, p2, p3);
        glm::vec3 trail_prev = CubicBezier(std::max(0.0f, t - 0.035f), p0, p1, p2, p3);
        glm::vec3 trail_direction = trail_next - trail_prev;
        float fade = 1.0f - age;
        float wave = sinf((config.animationTime * 2.0f) + (float)i * 0.70f);
        float intensity = 0.58f + 0.36f * fade;
        float length = 0.34f + 0.34f * fade;
        float width = 0.090f + 0.180f * fade;
        float height = 0.070f + 0.130f * fade;
        glm::mat4 cloud_basis = GuideBasisFromDirection(trail_position, trail_direction);

        glUniform1i(config.objectIdUniform, config.bezierTrailObjectId);
        DrawGuideSphere(config,
            cloud_basis * GuideScale(length, width, height),
            intensity,
            draw_object);
        DrawGuideSphere(config,
            cloud_basis * GuideTranslate(-0.060f, 0.055f * fade, 0.030f * wave * fade)
            * GuideScale(length * 0.92f, width * 0.76f, height * 0.88f),
            intensity * 0.94f,
            draw_object);
        DrawGuideSphere(config,
            cloud_basis * GuideTranslate(-0.110f, -0.050f * fade, -0.030f * wave * fade)
            * GuideScale(length * 0.84f, width * 0.72f, height * 0.80f),
            intensity * 0.90f,
            draw_object);
        DrawGuideSphere(config,
            cloud_basis * GuideTranslate(-0.170f, 0.015f * wave, 0.050f * fade)
            * GuideScale(length * 0.74f, width * 0.62f, height * 0.70f),
            intensity * 0.84f,
            draw_object);
    }

    float yaw = atan2f(-arrow_direction.z, arrow_direction.x);
    float pitch = asinf(std::max(-1.0f, std::min(1.0f, arrow_direction.y)));
    float arrow_pulse = 1.0f + 0.05f * sinf(config.animationTime * 5.0f);
    glm::mat4 arrow_basis = GuideTranslate(arrow_position.x, arrow_position.y, arrow_position.z)
                          * GuideRotateY(yaw)
                          * GuideRotateZ(pitch)
                          * GuideScale(arrow_pulse, arrow_pulse, arrow_pulse);

    glUniform1i(config.objectIdUniform, config.directionArrowObjectId);
    glUniform3f(config.guideColorUniform, 0.93f, 0.93f, 0.90f);

    DrawGuideSphere(config,
        arrow_basis * GuideTranslate(-0.50f, 0.0f, 0.0f) * GuideScale(0.62f, 0.20f, 0.20f),
        0.93f,
        draw_object);

    glUniform3f(config.guideColorUniform, 0.18f, 0.18f, 0.17f);
    DrawGuideObject(config,
        arrow_basis * GuideTranslate(0.20f, 0.0f, 0.0f) * GuideScale(1.06f, 1.06f, 1.18f),
        "guide_arrow_head",
        draw_object);

    glUniform3f(config.guideColorUniform, 0.96f, 0.96f, 0.92f);
    DrawGuideObject(config,
        arrow_basis * GuideTranslate(0.20f, 0.0f, 0.0f) * GuideScale(0.92f, 0.92f, 1.00f),
        "guide_arrow_head",
        draw_object);

    float pulse = 1.0f + 0.08f * sinf(config.animationTime * 3.2f);
    float door_x = exit_x - 0.25f;
    float door_z = exit_z;
    const float balcony_floor_top_y = config.wallHeight - 2.55f;
    const float frame_bottom_y = balcony_floor_top_y + 0.12f;
    const float frame_top_y = config.wallHeight - 0.22f;
    const float frame_center_y = (frame_bottom_y + frame_top_y) * 0.5f;
    const float frame_half_height = (frame_top_y - frame_bottom_y) * 0.5f;
    const float frame_half_width = 1.62f * pulse;

    glUniform1i(config.objectIdUniform, config.exitMarkerObjectId);
    glDisable(GL_DEPTH_TEST);

    DrawGuideCube(config,
        GuideTranslate(door_x, frame_center_y, door_z - frame_half_width) * GuideScale(0.075f, frame_half_height, 0.13f),
        draw_object);
    DrawGuideCube(config,
        GuideTranslate(door_x, frame_center_y, door_z + frame_half_width) * GuideScale(0.075f, frame_half_height, 0.13f),
        draw_object);
    DrawGuideCube(config,
        GuideTranslate(door_x, frame_top_y, door_z) * GuideScale(0.075f, 0.11f, frame_half_width + 0.18f),
        draw_object);
    glEnable(GL_DEPTH_TEST);
}








