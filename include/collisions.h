#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>

// Estrutura simples para caixas alinhadas aos eixos (AABB).
// O enunciado exige testes de interseccao com proposito dentro da aplicacao;
// usamos AABBs para limitar a area caminhavel da sala e para futuras colisoes
// com plataforma, mesa e pecas posicionadas.
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

// Resolve colisao horizontal entre o jogador (aproximado por um circulo no
// plano XZ) e uma AABB. A altura Y e preservada, pois o jogador atual nao pula
// nem sobe em objetos; o objetivo e impedir atravessar mesa e pecas.
glm::vec3 ResolveHorizontalCircleVsAABB(glm::vec3 point, float radius, const CollisionAABB& box);

// Teste de interseccao entre um raio e uma esfera simplificada.
// Ele e usado para a mira central: a camera emite um raio e as pecas
// selecionaveis sao aproximadas por esferas de raio proporcional a escala.
bool RayIntersectsSphere(
    glm::vec3 ray_origin,
    glm::vec3 ray_direction,
    glm::vec3 sphere_center,
    float sphere_radius,
    float max_distance,
    float* hit_depth
);

#endif
