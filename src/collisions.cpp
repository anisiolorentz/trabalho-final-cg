#include "collisions.h"

#include <cmath>
#include <algorithm>

CollisionAABB MakeAABBFromCenterHalfExtents(glm::vec3 center, glm::vec3 half_extents)
{
    CollisionAABB box;
    box.min = center - half_extents;
    box.max = center + half_extents;
    return box;
}

CollisionAABB MakeRoomWalkableAABB(float room_width, float room_depth, float wall_thickness, float player_radius)
{
    // A sala desenhada em main.cpp possui paredes centradas nas bordas.
    // Esta caixa representa somente a area interna caminhavel, ja descontando
    // a espessura da parede e um pequeno raio do jogador para evitar que a
    // camera encoste visualmente na geometria.
    float half_width = room_width / 2.0f;
    float half_depth = room_depth / 2.0f;
    float limit_x = half_width - wall_thickness - player_radius;
    float limit_z = half_depth - wall_thickness - player_radius;

    CollisionAABB box;
    box.min = glm::vec3(-limit_x, 0.0f, -limit_z);
    box.max = glm::vec3( limit_x, 3.0f,  limit_z);
    return box;
}

bool AABBIntersectsAABB(const CollisionAABB& a, const CollisionAABB& b)
{
    bool separated_x = a.max.x < b.min.x || a.min.x > b.max.x;
    bool separated_y = a.max.y < b.min.y || a.min.y > b.max.y;
    bool separated_z = a.max.z < b.min.z || a.min.z > b.max.z;

    return !(separated_x || separated_y || separated_z);
}

bool PointInsideAABB(glm::vec3 point, const CollisionAABB& box)
{
    return point.x >= box.min.x && point.x <= box.max.x
        && point.y >= box.min.y && point.y <= box.max.y
        && point.z >= box.min.z && point.z <= box.max.z;
}

glm::vec3 ClampPointToAABB(glm::vec3 point, const CollisionAABB& box)
{
    point.x = std::max(box.min.x, std::min(box.max.x, point.x));
    point.y = std::max(box.min.y, std::min(box.max.y, point.y));
    point.z = std::max(box.min.z, std::min(box.max.z, point.z));
    return point;
}

glm::vec3 ResolveHorizontalCircleVsAABB(glm::vec3 point, float radius, const CollisionAABB& box)
{
    // Primeiro encontramos o ponto da AABB mais proximo do centro do jogador,
    // considerando apenas XZ. Se a distancia ate esse ponto for menor que o
    // raio do jogador, existe interseccao e empurramos o centro para fora.
    float closest_x = std::max(box.min.x, std::min(box.max.x, point.x));
    float closest_z = std::max(box.min.z, std::min(box.max.z, point.z));

    float dx = point.x - closest_x;
    float dz = point.z - closest_z;
    float distance_sq = dx * dx + dz * dz;

    if (distance_sq > radius * radius)
        return point;

    // Caso comum: o centro esta fora da caixa, mas o circulo encostou nela.
    if (distance_sq > 1e-8f)
    {
        float distance = std::sqrt(distance_sq);
        float push = radius - distance;
        point.x += (dx / distance) * push;
        point.z += (dz / distance) * push;
        return point;
    }

    // Caso especial: o centro do jogador esta dentro da projecao XZ da caixa.
    // Escolhemos o lado mais proximo para empurrar o jogador para fora de forma
    // estavel, evitando travamento quando ele nasce ou anda dentro do obstaculo.
    float push_left  = std::fabs(point.x - box.min.x);
    float push_right = std::fabs(box.max.x - point.x);
    float push_back  = std::fabs(point.z - box.min.z);
    float push_front = std::fabs(box.max.z - point.z);

    float best = push_left;
    int side = 0;
    if (push_right < best) { best = push_right; side = 1; }
    if (push_back  < best) { best = push_back;  side = 2; }
    if (push_front < best) { best = push_front; side = 3; }

    if (side == 0) point.x = box.min.x - radius;
    if (side == 1) point.x = box.max.x + radius;
    if (side == 2) point.z = box.min.z - radius;
    if (side == 3) point.z = box.max.z + radius;

    return point;
}

bool RayIntersectsSphere(
    glm::vec3 ray_origin,
    glm::vec3 ray_direction,
    glm::vec3 sphere_center,
    float sphere_radius,
    float max_distance,
    float* hit_depth
)
{
    // A direcao deve estar normalizada para que "depth" esteja em unidades do
    // mundo. Mesmo assim, calculamos de forma robusta para evitar divisao por
    // zero se algum chamador passar um vetor degenerado.
    float dir_length_sq = ray_direction.x * ray_direction.x
                        + ray_direction.y * ray_direction.y
                        + ray_direction.z * ray_direction.z;
    if (dir_length_sq <= 1e-8f)
        return false;

    glm::vec3 to_center = sphere_center - ray_origin;
    float depth = (to_center.x * ray_direction.x
                 + to_center.y * ray_direction.y
                 + to_center.z * ray_direction.z) / std::sqrt(dir_length_sq);

    if (depth <= 0.0f || depth > max_distance)
        return false;

    glm::vec3 unit_direction = ray_direction / std::sqrt(dir_length_sq);
    glm::vec3 closest_point = ray_origin + unit_direction * depth;
    glm::vec3 offset = sphere_center - closest_point;
    float distance_sq = offset.x * offset.x
                      + offset.y * offset.y
                      + offset.z * offset.z;

    if (distance_sq > sphere_radius * sphere_radius)
        return false;

    if (hit_depth != 0)
        *hit_depth = depth;

    return true;
}
