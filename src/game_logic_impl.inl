bool GetRampLocalXZ(const GameObject& object, glm::vec3 player_position, float* local_x, float* local_z);
bool IsPlayerOnRampWalkableSurface(const GameObject& object, glm::vec3 player_position, float player_feet_y);

CollisionAABB GetGameObjectCollisionBox(const GameObject& object)
{
    
    
    
    
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
        max.y +=  1.45f * object.scale.y;
        min.z += -0.40f * object.scale.z;
        max.z +=  0.40f * object.scale.z;
    }
    else if (object.objectId == CYLINDER_PIECE)
    {
        min.x += -0.34f * object.scale.x;
        max.x +=  0.34f * object.scale.x;
        min.y +=  0.00f * object.scale.y;
        max.y +=  1.50f * object.scale.y;
        min.z += -0.34f * object.scale.z;
        max.z +=  0.34f * object.scale.z;
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
    
    
    
    CollisionAABB walkable_room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
    player_position = ClampPointToAABB(player_position, walkable_room);

    CollisionAABB table_box = MakeAABBFromCenterHalfExtents(
        TABLE_POSITION + glm::vec3(0.0f, TABLE_COLLIDER_HALF_EXTENTS.y, 0.0f),
        TABLE_COLLIDER_HALF_EXTENTS
    );
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
    if (player_feet_y < table_box.max.y - 0.05f)
        player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, table_box);

    const float balcony_wall_y = BALCONY_FLOOR_TOP_Y + BALCONY_SIDE_WALL_HEIGHT / 2.0f;
    const float balcony_wall_z_offset = BALCONY_FLOOR_HALF_EXTENTS.z - BALCONY_SIDE_WALL_HALF_EXTENTS.z;
    for (int side = -1; side <= 1; side += 2)
    {
        CollisionAABB balcony_wall = MakeAABBFromCenterHalfExtents(
            glm::vec3(BALCONY_FLOOR_CENTER.x, balcony_wall_y, side * balcony_wall_z_offset),
            BALCONY_SIDE_WALL_HALF_EXTENTS
        );
        player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
        if (player_feet_y < balcony_wall.max.y - 0.05f)
            player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, balcony_wall);
    }

    for (size_t i = 0; i < g_GameObjects.size(); ++i)
    {
        const GameObject& object = g_GameObjects[i];

        
        
        
        if (object.source || (int)i == g_HeldObjectIndex)
            continue;

        CollisionAABB object_box = GetGameObjectCollisionBox(object);
        player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
        float walkable_top = GetWalkableTopHeightForObject(object, object_box, player_position);
        if (object.objectId == TRIANGLE_PIECE)
        {
            if (!IsPlayerOnRampWalkableSurface(object, player_position, player_feet_y))
                player_position = ResolveHorizontalCircleVsAABB(player_position, PLAYER_RADIUS, object_box);
            continue;
        }

        float jump_height = (PLAYER_JUMP_SPEED * PLAYER_JUMP_SPEED) / (2.0f * GRAVITY_ACCELERATION);
        float climb_margin = std::min(jump_height + 0.10f, (object_box.max.y - object_box.min.y) + 0.10f);
        if (player_feet_y < walkable_top - climb_margin)
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

    
    
    
    float local_x = 0.0f;
    float local_z = 0.0f;
    GetRampLocalXZ(object, player_position, &local_x, &local_z);

    float t = (0.60f - local_x) / 1.20f;
    t = std::max(0.0f, std::min(1.0f, t));
    return object.position.y + (t * 1.45f * object.scale.y);
}

bool GetRampLocalXZ(const GameObject& object, glm::vec3 player_position, float* local_x, float* local_z)
{
    if (object.objectId != TRIANGLE_PIECE)
        return false;

    glm::vec3 relative = player_position - object.position;
    float c = cosf(-object.rotation.y);
    float s = sinf(-object.rotation.y);
    float local_x_scaled = c * relative.x + s * relative.z;
    float local_z_scaled = -s * relative.x + c * relative.z;

    *local_x = local_x_scaled / std::max(0.001f, object.scale.x);
    *local_z = local_z_scaled / std::max(0.001f, object.scale.z);
    return true;
}

bool IsPlayerOnRampWalkableSurface(const GameObject& object, glm::vec3 player_position, float player_feet_y)
{
    float local_x = 0.0f;
    float local_z = 0.0f;
    if (!GetRampLocalXZ(object, player_position, &local_x, &local_z))
        return false;

    const float entry_margin = 0.90f;
    const float side_margin = 0.14f;
    if (local_x < -0.60f - entry_margin || local_x > 0.60f + entry_margin)
        return false;
    if (std::fabs(local_z) > 0.40f + side_margin)
        return false;

    CollisionAABB object_box = GetGameObjectCollisionBox(object);
    float walkable_top = GetWalkableTopHeightForObject(object, object_box, player_position);
    return player_feet_y >= walkable_top - 0.28f && g_PlayerVerticalVelocity <= 1.25f;
}

float FindPlayerSupportHeight(glm::vec3 player_position){
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

    CollisionAABB balcony_floor = MakeAABBFromCenterHalfExtents(BALCONY_FLOOR_CENTER, BALCONY_FLOOR_HALF_EXTENTS);
    if (HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, balcony_floor)
        && player_feet_y >= balcony_floor.min.y - 0.40f
        && g_PlayerVerticalVelocity <= 1.25f)
    {
        support_height = std::max(support_height, balcony_floor.max.y);
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
        if (object.objectId == TRIANGLE_PIECE)
        {
            if (IsPlayerOnRampWalkableSurface(object, player_position, player_feet_y))
                support_height = std::max(support_height, walkable_top);
            continue;
        }

        float jump_height = (PLAYER_JUMP_SPEED * PLAYER_JUMP_SPEED) / (2.0f * GRAVITY_ACCELERATION);
        float step_margin = std::min(jump_height + 0.10f, (object_box.max.y - object_box.min.y) + 0.10f);

        
        
        
        if (player_feet_y >= walkable_top - step_margin && g_PlayerVerticalVelocity <= 1.25f)
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

        if (object.grounded)
        {
            CollisionAABB probe_box = before_box;
            probe_box.min.y -= 0.06f;
            probe_box.max.y -= 0.06f;
            float resting_support = FindSupportHeightForObject(i, before_box, probe_box);
            if (before_box.min.y <= resting_support + 0.08f)
            {
                object.position.y += resting_support + OBJECT_REST_EPSILON - before_box.min.y;
                object.verticalVelocity = 0.0f;
                object.grounded = true;
                continue;
            }
        }

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
         + vertical_factor * vertical_factor * 13.20f
         + vertical_factor * distance_factor * 2.35f;
}

float ComputeHeldObjectMaxScaleRatio(const GameObject& object)
{
    GameObject base_object = object;
    base_object.scale = object.baseScale;
    CollisionAABB base_box = GetGameObjectCollisionBox(base_object);

    float base_width = std::max(0.01f, base_box.max.x - base_box.min.x);
    float base_height = std::max(0.01f, base_box.max.y - base_box.min.y);
    float base_depth = std::max(0.01f, base_box.max.z - base_box.min.z);

    CollisionAABB room = MakeRoomWalkableAABB(ROOM_WIDTH, ROOM_DEPTH, WALL_HEIGHT, WALL_THICK, PLAYER_RADIUS);
    float room_width = std::max(0.01f, room.max.x - room.min.x);
    float room_depth = std::max(0.01f, room.max.z - room.min.z);

    const float MAX_HELD_SCALE_RATIO = 5.75f;
    float room_limited_ratio = std::min(room_width / base_width, room_depth / base_depth) * 0.48f;
    float height_limited_ratio = (WALL_HEIGHT - 0.40f) / base_height;

    return std::max(1.0f, std::min(MAX_HELD_SCALE_RATIO, std::min(room_limited_ratio, height_limited_ratio)));
}
float ComputeHeldObjectMaxDistance(const GameObject& object, float preferred_distance)
{
    if (g_CameraForward.y <= 0.02f)
        return preferred_distance;

    CollisionAABB object_box = GetGameObjectCollisionBox(object);
    float object_height = std::max(0.01f, object_box.max.y - object_box.min.y);
    float max_origin_y = WALL_HEIGHT - 0.25f - object_height;
    float camera_y = g_CameraPosition.y;

    if (max_origin_y <= camera_y)
        return std::min(preferred_distance, 1.25f);

    float ceiling_distance = (max_origin_y - camera_y) / g_CameraForward.y;
    return std::max(1.25f, std::min(preferred_distance, ceiling_distance));
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
        g_HeldHasOriginalObject = false;
        g_HeldWasSourceCopy = false;
        g_HeldYawOffset = 0.0f;
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
            
            
            GameObject heldCopy = g_GameObjects[g_SelectedObjectIndex];
            heldCopy.selectable = false;
            heldCopy.source = false;
            heldCopy.physicsEnabled = false;
            heldCopy.grounded = false;
            heldCopy.verticalVelocity = 0.0f;
            heldCopy.baseScale = heldCopy.scale;
            g_GameObjects.push_back(heldCopy);
            g_HeldObjectIndex = (int)g_GameObjects.size() - 1;
            g_HeldOriginalObject = heldCopy;
            g_HeldHasOriginalObject = true;
            g_HeldWasSourceCopy = true;
        }
        else
        {
            
            
            g_HeldObjectIndex = g_SelectedObjectIndex;
            g_HeldOriginalObject = g_GameObjects[g_HeldObjectIndex];
            g_HeldHasOriginalObject = true;
            g_HeldWasSourceCopy = false;
            g_GameObjects[g_HeldObjectIndex].selectable = false;
            g_GameObjects[g_HeldObjectIndex].physicsEnabled = false;
            g_GameObjects[g_HeldObjectIndex].grounded = false;
            g_GameObjects[g_HeldObjectIndex].verticalVelocity = 0.0f;
            g_GameObjects[g_HeldObjectIndex].baseScale = g_GameObjects[g_HeldObjectIndex].scale;
        }

        
        
        
        
        g_HeldYawOffset = g_GameObjects[g_HeldObjectIndex].rotation.y - ComputeYawFromDirection(g_CameraForward);
        g_HeldReferenceScaleFactor = ComputeHeldObjectScaleFactor();
        g_SelectedObjectIndex = g_HeldObjectIndex;
        printf("Segurando objeto: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
        fflush(stdout);
    }
}



















void CancelHeldObject()
{
    if (g_HeldObjectIndex < 0)
    {
        g_SelectedObjectIndex = -1;
        return;
    }

    printf("Manipulacao cancelada: %s\n", g_GameObjects[g_HeldObjectIndex].meshName.c_str());
    fflush(stdout);

    if (g_HeldWasSourceCopy)
    {
        g_GameObjects.erase(g_GameObjects.begin() + g_HeldObjectIndex);
    }
    else if (g_HeldHasOriginalObject)
    {
        g_GameObjects[g_HeldObjectIndex] = g_HeldOriginalObject;
        g_GameObjects[g_HeldObjectIndex].selectable = true;
        g_GameObjects[g_HeldObjectIndex].movable = true;
    }

    g_HeldObjectIndex = -1;
    g_SelectedObjectIndex = -1;
    g_HeldHasOriginalObject = false;
    g_HeldWasSourceCopy = false;
}


void ResetGameState()
{
    g_GameObjects.clear();

    GameObject cube;
    cube.meshName = "puzzle_cube";
    cube.objectId = CUBE_PIECE;
    cube.position = TABLE_POSITION + glm::vec3(-0.75f, 1.61f, 0.10f);
    cube.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    cube.scale = glm::vec3(0.35f, 0.35f, 0.35f);
    cube.baseScale = cube.scale;
    cube.selectable = true;
    cube.movable = true;
    cube.source = true;
    cube.physicsEnabled = false;
    cube.grounded = true;
    cube.verticalVelocity = 0.0f;
    g_GameObjects.push_back(cube);

    GameObject triangularPiece;
    triangularPiece.meshName = "puzzle_triangular_piece";
    triangularPiece.objectId = TRIANGLE_PIECE;
    triangularPiece.position = TABLE_POSITION + glm::vec3(0.05f, 1.61f, 0.08f);
    triangularPiece.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    triangularPiece.scale = glm::vec3(0.42f, 0.42f, 0.42f);
    triangularPiece.baseScale = triangularPiece.scale;
    triangularPiece.selectable = true;
    triangularPiece.movable = true;
    triangularPiece.source = true;
    triangularPiece.physicsEnabled = false;
    triangularPiece.grounded = true;
    triangularPiece.verticalVelocity = 0.0f;
    g_GameObjects.push_back(triangularPiece);

    GameObject cylinder;
    cylinder.meshName = "puzzle_cylinder";
    cylinder.objectId = CYLINDER_PIECE;
    cylinder.position = TABLE_POSITION + glm::vec3(0.82f, 1.61f, 0.08f);
    cylinder.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    cylinder.scale = glm::vec3(0.45f, 0.45f, 0.45f);
    cylinder.baseScale = cylinder.scale;
    cylinder.selectable = true;
    cylinder.movable = true;
    cylinder.source = true;
    cylinder.physicsEnabled = false;
    cylinder.grounded = true;
    cylinder.verticalVelocity = 0.0f;
    g_GameObjects.push_back(cylinder);

    g_SelectedObjectIndex = -1;
    g_HeldObjectIndex = -1;
    g_HeldReferenceScaleFactor = 1.0f;
    g_HeldHasOriginalObject = false;
    g_HeldWasSourceCopy = false;
    g_CameraTheta = INITIAL_CAMERA_THETA;
    g_CameraPhi = INITIAL_CAMERA_PHI;
    g_CameraPosition = INITIAL_CAMERA_POSITION;
    g_CameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    g_PlayerVerticalVelocity = 0.0f;
    g_PlayerGrounded = true;
    g_KeyW_Pressed = false;
    g_KeyA_Pressed = false;
    g_KeyS_Pressed = false;
    g_KeyD_Pressed = false;
    g_BezierGuideT = 0.0f;
    g_ExitAnimTime = 0.0f;
    g_GameWon = false;
    g_VictoryTimer = 0.0f;
}

void UpdateVictoryState()
{
    if (g_GameWon)
    {
        g_VictoryTimer += g_DeltaTime;
        if (g_VictoryTimer >= 10.0f)
            ResetGameState();
        return;
    }

    glm::vec3 player_position(g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);
    float player_feet_y = player_position.y - PLAYER_EYE_HEIGHT;
    CollisionAABB balcony_floor = MakeAABBFromCenterHalfExtents(BALCONY_FLOOR_CENTER, BALCONY_FLOOR_HALF_EXTENTS);

    if (HorizontalCircleOverlapsAABB(player_position, PLAYER_RADIUS, balcony_floor)
        && player_feet_y >= balcony_floor.max.y - 0.08f
        && player_feet_y <= balcony_floor.max.y + 0.20f
        && g_PlayerVerticalVelocity <= 0.20f)
    {
        g_GameWon = true;
        g_VictoryTimer = 0.0f;
        g_HeldObjectIndex = -1;
        g_SelectedObjectIndex = -1;
        g_PlayerVerticalVelocity = 0.0f;
    }
}




