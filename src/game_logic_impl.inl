CollisionAABB GetGameObjectCollisionBox(const GameObject& object)
{
    // As malhas das pecas foram modeladas com a origem na base (y = 0), nao no
    // centro. Por isso a AABB precisa partir de position.y como "chao" da peca;
    // se tratarmos position como centro, a correcao de gravidade empurra a peca
    // para cima e ela parece flutuar.
    glm::vec3 min = object.position;
    glm::vec3 max = object.position;

    if (object.objectId == CUBE_PIECE)
    {
        min.x += -0.50f * object.scale.x;
        max.x +=  0.50f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  1.00f * object.scale.y;
        min.z += -0.50f * object.scale.z;
        max.z +=  0.50f * object.scale.z;
    }
    else if (object.objectId == TRIANGLE_PIECE)
    {
        min.x += -0.60f * object.scale.x;
        max.x +=  0.60f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  0.80f * object.scale.y;
        min.z += -0.40f * object.scale.z;
        max.z +=  0.40f * object.scale.z;
    }
    else if (object.objectId == CYLINDER_PIECE)
    {
        min.x += -0.50f * object.scale.x;
        max.x +=  0.50f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  1.50f * object.scale.y;
        min.z += -0.50f * object.scale.z;
        max.z +=  0.50f * object.scale.z;
    }
    else
    {
        min += glm::vec3(-0.5f * object.scale.x, 0.0f, -0.5f * object.scale.z);
        max += glm::vec3( 0.5f * object.scale.x, object.scale.y, 0.5f * object.scale.z);
    }

    CollisionAABB box;
    box.min = min;
    box.max = max;
    return box;
}

glm::vec3 ResolvePlayerCollisions(glm::vec3 player_position)
{
    // Primeiro prendemos o jogador dentro da area interna da sala retangular.
    // Depois resolvemos obstaculos internos e prendemos novamente, pois uma
    // colisao perto da parede poderia empurrar o jogador para fora da sala.
    CollisionAABB walkable_room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
    player_position = ClampPointToAABB(player_position, walkable_room);

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
    if (player_feet_y < table_box.max.y - 0.05f)
        player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, table_box);

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];

        // As pecas originais em cima da mesa sao fontes reutilizaveis e ficam
        // cobertas pela colisao maior da mesa. A peca segurada tambem nao deve
        // bloquear o jogador, pois ela acompanha a camera.
        if (object.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB object_box = GetGameObjectCollisionBox(object);
        player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
        if (player_feet_y < object_box.max.y - 0.05f)
            player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, object_box);
    }

    player_position = ClampPointToAABB(player_position, walkable_room);
    return player_position;
}

bool HorizontalCircleOverlapsAABB(glm::vec3 center, float radius, const CollisionAABB& box)
{
    float closest_x = std::max(box.min.x, std::min(box.max.x, center.x));
    float closest_z = std::max(box.min.z, std::min(box.max.z, center.z));
    float dx = center.x - closest_x;
    float dz = center.z - closest_z;
    return dx * dx + dz * dz <= radius * radius;
}

float GetWalkableTopHeightForObject(const GameObject& object, const CollisionAABB& object_box, glm::vec3 player_position)
{
    if (object.objectId != TRIANGLE_PIECE)
        return object_box.max.y;

    // A rampa triangular do OBJ sobe ao longo do eixo X local: em x=-0.6 a
    // altura e 0.8, em x=+0.6 a altura e 0.0. Como atualmente as pecas nao sao
    // rotacionadas pela interacao, usamos esse mapeamento direto para permitir
    // caminhar pela inclinacao em vez de tratar a rampa como um bloco.
    float t = (object_box.max.x - player_position.x) / std::max(0.001f, object_box.max.x - object_box.min.x);
    t = std::max(0.0f, std::min(1.0f, t));
    return object_box.min.y + t * (object_box.max.y - object_box.min.y);
}

float FindPlayerSupportHeight(glm::vec3 player_position)
{
    float support_height = 0.0f;
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    if (HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, table_box)
        && player_feet_y >= table_box.max.y - 0.20f)
    {
        support_height = std::max(support_height, table_box.max.y);
    }

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        if ((int)i == g_HeldObjectIndex)
            continue;

        const GameObject& object = g_GameObjects[i];
        if (object.source)
            continue;

        CollisionAABB object_box = GetGameObjectCollisionBox(object);
        if (!HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, object_box))
            continue;

        float walkable_top = GetWalkableTopHeightForObject(object, object_box, player_position);
        float step_margin = (object.objectId == TRIANGLE_PIECE) ? 0.35f : 0.20f;

        // O jogador pode caminhar sobre a peça quando seus pés estão próximos
        // ou acima da superfície caminhável. Para a rampa, essa superfície é
        // inclinada; para os demais objetos, é o topo da AABB.
        if (player_feet_y >= walkable_top - step_margin)
            support_height = std::max(support_height, walkable_top);
    }

    return support_height;
}

void UpdatePlayerVerticalPhysics()
{
    glm::vec3 player_position(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
    float support_height = FindPlayerSupportHeight(player_position);
    float target_eye_height = support_height + PLAYER_EYE_HEIGHT;

    g_PlayerVerticalVelocity -= GRAVITY_ACCELERATION * g_DeltaTime;
    player_position.y += g_PlayerVerticalVelocity * g_DeltaTime;

    if (player_position.y <= target_eye_height)
    {
        player_position.y = target_eye_height;
        g_PlayerVerticalVelocity = 0.0f;
        g_PlayerGrounded = true;
    }
    else
    {
        g_PlayerGrounded = false;
    }

    g_CameraPosition.y = player_position.y;
}

bool HorizontalAABBOverlap(const CollisionAABB& a, const CollisionAABB& b)
{
    bool separated_x = a.max.x <= b.min.x || a.min.x >= b.max.x;
    bool separated_z = a.max.z <= b.min.z || a.min.z >= b.max.z;
    return !(separated_x || separated_z);
}

float FindSupportHeightForObject(size_t object_index, const CollisionAABB& before_box, const CollisionAABB& after_box)
{
    // O suporte mais básico é o chão da sala. A gravidade nunca deixa a peça
    // passar abaixo desse plano, mantendo a interação estável e previsível.
    float support_height = 0.0f;

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    if (HorizontalAABBOverlap(after_box, table_box)
        && before_box.max.y > table_box.max.y
        && after_box.min.y <= table_box.max.y)
    {
        support_height = std::max(support_height, table_box.max.y);
    }

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        if (i == object_index || (int)i == g_HeldObjectIndex)
            continue;

        const GameObject& other = g_GameObjects[i];
        if (other.source)
            continue;

        CollisionAABB other_box = GetGameObjectCollisionBox(other);
        if (!HorizontalAABBOverlap(after_box, other_box))
            continue;

        // A peca pousa se, durante este frame, sua base cruzou ou entrou no
        // topo de outra peca. Usamos before_box.max.y > top para garantir que
        // estamos tratando um suporte abaixo da peca, e nao algo acima dela.
        if (before_box.max.y > other_box.max.y && after_box.min.y <= other_box.max.y)
            support_height = std::max(support_height, other_box.max.y);
    }

    return support_height;
}

void UpdateDroppedObjectPhysics()
{
    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        GameObject& object = g_GameObjects[i];
        if (!object.physicsEnabled || object.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB before_box = GetGameObjectCollisionBox(object);

        object.verticalVelocity -= GRAVITY_ACCELERATION * g_DeltaTime;
        object.position.y += object.verticalVelocity * g_DeltaTime;

        CollisionAABB after_box = GetGameObjectCollisionBox(object);
        float support_height = FindSupportHeightForObject(i, before_box, after_box);
        if (after_box.min.y <= support_height)
        {
            object.position.y += support_height + OBJECT_REST_EPSILON - after_box.min.y;
            object.verticalVelocity = 0.0f;
            object.grounded = true;
        }
        else
        {
            object.grounded = false;
        }
    }
}

float ComputeHeldObjectScaleFactor()
{
    float vertical_factor = (g_CameraForward.y + 1.0f) / 2.0f;
    vertical_factor = std::max(0.0f, std::min(1.0f, vertical_factor));

    float distance_from_table = glm::length(glm::vec3(
        g_CameraPosition.x - TABLE_POSITION.x,
        0.0f,
        g_CameraPosition.z - TABLE_POSITION.z));
    float distance_factor = std::max(0.0f, std::min(1.0f, (distance_from_table - 4.0f) / 10.0f));

    return 0.10f
         + vertical_factor * vertical_factor * 11.90f
         + vertical_factor * distance_factor * 2.00f;
}

float ComputeHeldObjectDistance()
{
    float vertical_factor = (g_CameraForward.y + 1.0f) / 2.0f;
    vertical_factor = std::max(0.0f, std::min(1.0f, vertical_factor));

    float distance_from_table = glm::length(glm::vec3(
        g_CameraPosition.x - TABLE_POSITION.x,
        0.0f,
        g_CameraPosition.z - TABLE_POSITION.z));
    float distance_factor = std::max(0.0f, std::min(1.0f, (distance_from_table - 4.0f) / 10.0f));

    return 5.75f
         + vertical_factor * vertical_factor * 4.25f
         + vertical_factor * distance_factor * 1.50f;
}

float ComputeYawFromDirection(glm::vec3 direction)
{
    glm::vec3 horizontal(direction.x, 0.0f, direction.z);
    float len = sqrtf(horizontal.x * horizontal.x + horizontal.z * horizontal.z);
    if (len <= 1e-6f)
        return 0.0f;

    horizontal /= len;
    return atan2f(-horizontal.x, -horizontal.z);
}

int FindTargetedGameObject()
{
    glm::vec3 camera_position(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
    int best_index = -1;
    float best_depth = std::numeric_limits<float>::max();

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];
        if (!object.selectable)
            continue;

        float object_radius = std::max(object.scale.x, std::max(object.scale.y, object.scale.z)) * 0.8f + 0.20f;
        float depth = 0.0f;

        // Interseccao da mira central com a peca: aproximamos cada objeto por
        // uma esfera. Isso e suficiente para o puzzle atual e deixa a selecao
        // com proposito claro dentro da logica da aplicacao.
        if (RayIntersectsSphere(camera_position, g_CameraForward, object.position, object_radius, 8.0f, &depth)
            && depth < best_depth)
        {
            best_depth = depth;
            best_index = (int)i;
        }
    }

    return best_index;
}

void SelectNextGameObject()
{
    if (g_GameObjects.empty())
        return;

    if (g_HeldObjectIndex >= 0)
        return;

    int targeted_index = FindTargetedGameObject();
    if (targeted_index >= 0)
    {
        g_SelectedObjectIndex = targeted_index;
        printf("Objeto selecionado pela mira: %s\n", g_GameObjects[targeted_index].meshName.c_str());
        fflush(stdout);
        return;
    }

    int start = g_SelectedObjectIndex;
    for (size_t offset = 1; offset <= g_GameObjects.size(); ++offset)
    {
        int candidate = (start + (int)offset) % (int)g_GameObjects.size();
        if (g_GameObjects[candidate].selectable)
        {
            g_SelectedObjectIndex = candidate;
            printf("Objeto selecionado: %s\n", g_GameObjects[candidate].meshName.c_str());
            fflush(stdout);
            return;
        }
    }
}

void ToggleHeldObject()
{
    if (g_HeldObjectIndex >= 0)
    {
        printf("Objeto solto: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
        fflush(stdout);
        g_GameObjects[g_HeldObjectIndex].selectable = true;
        g_GameObjects[g_HeldObjectIndex].movable = true;
        g_GameObjects[g_HeldObjectIndex].source = false;
        g_GameObjects[g_HeldObjectIndex].physicsEnabled = true;
        g_GameObjects[g_HeldObjectIndex].grounded = false;
        g_GameObjects[g_HeldObjectIndex].verticalVelocity = 0.0f;
        g_HeldObjectIndex = -1;
        g_SelectedObjectIndex = -1;
        return;
    }

    if (g_SelectedObjectIndex < 0)
        g_SelectedObjectIndex = FindTargetedGameObject();

    if (g_SelectedObjectIndex >= 0 && g_GameObjects[g_SelectedObjectIndex].movable)
    {
        if (g_GameObjects[g_SelectedObjectIndex].source)
        {
            // Pecas originais sobre a mesa sao fontes reutilizaveis: pegar uma
            // delas cria uma copia segurada e deixa a fonte no lugar.
            GameObject heldCopy = g_GameObjects[g_SelectedObjectIndex];
            heldCopy.selectable = false;
            heldCopy.source = false;
            heldCopy.physicsEnabled = false;
            heldCopy.grounded = false;
            heldCopy.verticalVelocity = 0.0f;
            heldCopy.baseScale = heldCopy.scale;
            g_GameObjects.push_back(heldCopy);
            g_HeldObjectIndex = (int)g_GameObjects.size() - 1;
        }
        else
        {
            // Pecas ja soltas no mundo podem ser pegas novamente. Nesse caso,
            // seguramos o proprio objeto, sem duplicar a malha logica.
            g_HeldObjectIndex = g_SelectedObjectIndex;
            g_GameObjects[g_HeldObjectIndex].selectable = false;
            g_GameObjects[g_HeldObjectIndex].physicsEnabled = false;
            g_GameObjects[g_HeldObjectIndex].grounded = false;
            g_GameObjects[g_HeldObjectIndex].verticalVelocity = 0.0f;
            g_GameObjects[g_HeldObjectIndex].baseScale = g_GameObjects[g_HeldObjectIndex].scale;
        }

        // Guardamos o fator de escala do exato momento em que o jogador pega
        // a peça. Assim, no primeiro frame segurando, a razão é 1.0 e o objeto
        // aparece com o mesmo tamanho que tinha sobre a mesa/no mundo. A escala
        // só passa a mudar quando o jogador anda ou altera a direção da câmera.
        g_HeldReferenceScaleFactor = ComputeHeldObjectScaleFactor();
        g_SelectedObjectIndex = g_HeldObjectIndex;
        printf("Segurando objeto: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
        fflush(stdout);
    }
}

