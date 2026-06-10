#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>





struct CollisionAABB
{
    glm::vec3 min;
    glm::vec3 max;
};

CollisionAABB MakeAABBFromCenterHalfExtents(glm::vec3 center, glm::vec3 half_extents);
CollisionAABB MakeRoomWalkableAABB(float room_width, float room_depth, float wall_height, float wall_thickness, float player_radius);

bool AABBIntersectsAABB(const CollisionAABB& a, const CollisionAABB& b);
bool PointInsideAABB(glm::vec3 point, const CollisionAABB& box);
glm::vec3 ClampPointToAABB(glm::vec3 point, const CollisionAABB& box);




glm::vec3 ResolveHorizontalCircleVsAABB(glm::vec3 point, float radius, const CollisionAABB& box);




bool RayIntersectsSphere(
    glm::vec3 ray_origin,
    glm::vec3 ray_direction,
    glm::vec3 sphere_center,
    float sphere_radius,
    float max_distance,
    float* hit_depth
);

#endif

