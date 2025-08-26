#include "world.h"
#include "august.h"

file_private 
U32 calculate_hash_for_chunk_pos(World* world, Vec2_S32 chunk_pos) {
    // TOOD: get a better hash function
    U32 result = Abs(chunk_pos.x) * 73 + Abs(chunk_pos.y) * 31;
    return result % SizeOfArr(world->chunk_lists);  
}

Chunk* get_chunk_in_world(Arena* world_arena, World* world, Vec2_S32 chunk_pos) {
    U32 hash = calculate_hash_for_chunk_pos(world, chunk_pos);
    Assert(hash < SizeOfArr(world->chunk_lists));

    Chunk* result = 0;

    Chunk_list* chunk_list = &world->chunk_lists[hash];
    if (chunk_list->is_initialised)
    {
        Assert(chunk_list->chunk);
        Chunk* last_chunk = chunk_list->chunk;
        for (Chunk* it_chunk = chunk_list->chunk; 
            it_chunk != 0; 
            it_chunk = it_chunk->next
        ) {
            last_chunk = it_chunk;
            
            if (it_chunk->chunk_pos == chunk_pos) {
                result = it_chunk;
                break;
            }
        }
        if (result) {
            return result;
        }
        else {
            Chunk* new_chunk = ArenaPush(world_arena, Chunk);
            new_chunk->next = 0;
            new_chunk->chunk_pos = chunk_pos;
            new_chunk->tiles = ArenaPushArr(world_arena, U32, Square(world->chunk_side_in_tiles));
    
            last_chunk->next = new_chunk;
            result = new_chunk;
        }

    }
    else
    {
        chunk_list->is_initialised = true;
        chunk_list->chunk = ArenaPush(world_arena, Chunk); 
        chunk_list->chunk->next = 0;
        chunk_list->chunk->chunk_pos = chunk_pos;
        chunk_list->chunk->tiles = ArenaPushArr(world_arena, U32, Square(world->chunk_side_in_tiles));
        
        result = chunk_list->chunk;
    }

    Assert(result);
    return result;
}

Vec2_S32 tile_pos_from_chunk_rel(World* world, Vec2_F32 chunk_rel) {
    Vec2_S32 result = {};
    result.x = floor_F32_to_S32(chunk_rel.x / world->tile_side_in_m);
    result.y = floor_F32_to_S32(chunk_rel.y / world->tile_side_in_m);
    return result;
}

// ====================================================================================
B32 is_world_pos_canonical(World* world, World_pos test_pos) {
    B32 chunk_rel_valid = (   test_pos.chunk_rel.x >= 0
                           && test_pos.chunk_rel.y >= 0
                           && test_pos.chunk_rel.x < world->chunk_side_in_m
                           && test_pos.chunk_rel.y < world->chunk_side_in_m
                          );
    B32 chunk_valid = (   test_pos.chunk.x >= -world->chunk_safe_margin_from_middle_chunk
                        && test_pos.chunk.x <= (world->mid_chunk.x + world->chunk_safe_margin_from_middle_chunk)
                        && test_pos.chunk.y >= -world->chunk_safe_margin_from_middle_chunk
                        && test_pos.chunk.y <= (world->mid_chunk.y + world->chunk_safe_margin_from_middle_chunk)
                       );
    B32 is_valid = (   chunk_rel_valid 
                     && chunk_valid   
                    );
    
    if (!is_valid) {
        int x = 0;
    }
    return is_valid;
}

void recanonicalize_world_pos(World* world, World_pos* invalid_pos) {
    S32 chunk_offset_x = floor_F32_to_S32(invalid_pos->chunk_rel.x / world->chunk_side_in_m);
    S32 chunk_offset_y = floor_F32_to_S32(invalid_pos->chunk_rel.y / world->chunk_side_in_m);

    invalid_pos->chunk_rel.x -= chunk_offset_x * world->chunk_side_in_m;
    invalid_pos->chunk_rel.y -= chunk_offset_y * world->chunk_side_in_m;

    invalid_pos->chunk.x += chunk_offset_x;
    invalid_pos->chunk.y += chunk_offset_y;
    
    Assert(is_world_pos_canonical(world, *invalid_pos));
};

World_pos recanonicalize_world_pos_copy(World* world, World_pos* invalid_pos) {
    World_pos result = *invalid_pos;
    recanonicalize_world_pos(world, &result);
    return result;
};

void move_world_pos_by_n_meters(World* world, World_pos* pos, Vec2_F32 m) {
    pos->chunk_rel += m;
    recanonicalize_world_pos(world, pos);
}

World_pos move_world_pos_by_n_meters_copy(World* world, World_pos* pos, Vec2_F32 m) {
    World_pos result = *pos;
    move_world_pos_by_n_meters(world, &result, m);
    return result;
}

Vec2_F32 subtract_world_pos(World* world, World_pos* pos1, World_pos* pos2) {
    Vec2_F32 chunk_diff = vec2_f32_from_vec2_s32(pos1->chunk - pos2->chunk);
    Vec2_F32 chunk_rel_diff = pos1->chunk_rel - pos2->chunk_rel;


    Vec2_F32 result = (chunk_diff * vec2_f32(world->chunk_side_in_m, world->chunk_side_in_m)) + 
                      chunk_rel_diff;
    return result;
}

// ====================================================================================

// NOTE: just made this to have the name for the operation
file_private
World_pos world_pos_from_camera_rel(World* world, Camera* camera, Vec2_F32 camera_rel)
{
    World_pos result = move_world_pos_by_n_meters_copy(world, &camera->world_pos, camera_rel);
    return result;
}

file_private
B32 is_world_pos_within_camera_bounds(World* world, Camera* camera, World_pos test_pos)
{
    // NOTE: it just has to be within camera chunk space +- in up_down and left_right directions
    Vec2_S32 chunk_diff = test_pos.chunk - camera->world_pos.chunk; 

    B32 is_valid = (   chunk_diff.x >= -camera->chunks_from_camera_chunk
                    && chunk_diff.x <= camera->chunks_from_camera_chunk
                    && chunk_diff.y >= -camera->chunks_from_camera_chunk
                    && chunk_diff.y <= camera->chunks_from_camera_chunk
                   );
    return is_valid;
}

file_private
B32 is_camera_rel_pos_withing_camera_bounds(World* world, Camera* camera, Vec2_F32 camera_rel)
{
    World_pos test_world_pos = world_pos_from_camera_rel(world, camera, camera_rel);
    B32 is_within = is_world_pos_within_camera_bounds(world, camera, test_world_pos);
    return is_within;
}

file_private
Vec2_F32 camera_rel_pos_from_world_pos(World* world, Camera* camera, World_pos world_pos)
{
    Vec2_F32 camera_rel = subtract_world_pos(world, &world_pos, &camera->world_pos);
    Assert(is_camera_rel_pos_withing_camera_bounds(world, camera, camera_rel));
    return camera_rel;
}

// == Entity stuff here 
Entity get_entity(World* world, U32 entity_idx) {
    Assert(entity_idx < world->entity_count);

    Entity result = {};
    result.low = &world->low_entities[entity_idx];
    result.high = &world->high_entities[entity_idx];

    return result;
}

void spawn_wall(World* world, Camera* camera, World_pos wall_pos, F32 wall_width, F32 wall_height) {
    // Chunk* chunk = get_chunk_in_world(arena, world, pos.chunk);
    Assert(world->entity_count + 1 < world->entity_max_count);
    
    Low_entity low = {};
    low.width = wall_width;
    low.height = wall_height;
    low.speed = vec2_f32(0.0f, 0.0f);
    low.world_pos = wall_pos;
    low.is_also_high = is_world_pos_within_camera_bounds(world, camera, wall_pos);
    
    High_entity high = {};
    if (low.is_also_high)
    {
        high.camera_rel_pos = camera_rel_pos_from_world_pos(world, camera, low.world_pos);
        high.direction = Entity_direction::Back;
    }

    world->low_entities[world->entity_count]  = low;
    world->high_entities[world->entity_count] = high;
    world->entity_count += 1;
}

void change_entity_frequencies_based_on_camera_pos(World* world, Camera* camera)
{
    for (U32 entity_idx = 0;
         entity_idx < world->entity_count;
         entity_idx += 1
    ) {
        Low_entity* low   = &world->low_entities[entity_idx];
        High_entity* high = &world->high_entities[entity_idx];

        // NOTE: is was low, but now is in the high sim range
        if (   !low->is_also_high 
            && is_world_pos_within_camera_bounds(world, camera, low->world_pos)
        ) {
            low->is_also_high = true;
            high->camera_rel_pos = camera_rel_pos_from_world_pos(world, camera, low->world_pos);
            high->direction = high->direction; // This is here, to just not forget that direction exists
        }  
        else if (   
            low->is_also_high
            && !is_world_pos_within_camera_bounds(world, camera, low->world_pos)
        ) {
            low->is_also_high = false;
            
        }
        else {
            InvalidCodePath
        }
    }   

}



// =========================================
// =========================================
// =========================================
// =========================================

// file_private
struct Ray_intersection_result {
    B32 alredy_colliding;
    B32 will_colide;
    F32 travel_time_normalized;
};

file_private
Ray_intersection_result get_ray_intersection_result(
    F32 start_main_coord, F32 start_other_coord,
    F32 displacement_main_coord, F32 displacement_other_coord, 
    F32 wall_main_coord,
    F32 wall_other_min_coord, F32 wall_other_max_coord
) {
    Ray_intersection_result result = {};

    if (displacement_main_coord != 0.0f) 
    {   
        F32 time_to_hit_an_inf_wall_at_main_coord = (wall_main_coord - start_main_coord) / displacement_main_coord;
        
        if (   (time_to_hit_an_inf_wall_at_main_coord == -0.0f)
            || (time_to_hit_an_inf_wall_at_main_coord == 0.0f)
        ) {
            // TODO: go to see Casey's TestWall inside handmade.cpp (episode 054).
            //       why does his version never checks for -0.0f time to hit 
            //       and therefore goes throught a wall
            result.alredy_colliding       = true;
            result.will_colide            = false;
            result.travel_time_normalized =  0.0f;
        } 
        else if (time_to_hit_an_inf_wall_at_main_coord > 0.0f)
        {
            F32 possible_displacement_other_coord = (time_to_hit_an_inf_wall_at_main_coord * displacement_other_coord); 
            F32 wall_other_coord = start_other_coord + possible_displacement_other_coord;
            
            if (   wall_other_coord >= wall_other_min_coord
                && wall_other_coord <= wall_other_max_coord
            ) {
                result.alredy_colliding       = false;
                result.will_colide            = true;
                result.travel_time_normalized = time_to_hit_an_inf_wall_at_main_coord;
            }
        }
    }
    
    return result;
}

void move_player(World* world, 
                 Player* player, 
                 Camera* camera,
                 F32 time_elapsed,
                 Vec2_F32 acceleration_unit_vec
) {
    F32 t = time_elapsed;
    F32 acceleration_multiplier = 150;

    Vec2_F32 a = acceleration_unit_vec * acceleration_multiplier;
    Vec2_F32 new_speed = player->speed + (a * t);
    Vec2_F32 displacement = (a/2 * t*t) + (player->speed * t);
    new_speed /= 1.5f; // NOTE: this is something like friction but a very vanila hardcoded version

    if (displacement != vec2_f32(0.0f, 0.0f))
    {
        F32 min_time = 1.0f;
        Vec2_S32 wall_hit_direction = {};

        for (U32 entity_idx = 0; 
             entity_idx < world->entity_count; 
             entity_idx += 1
        ) {
            Entity test_entity = get_entity(world, entity_idx);

            #if 1
            {
                World_pos player_world_pos = world_pos_from_camera_rel(world, camera, player->camera_rel);
                if (test_entity.low->world_pos.chunk == player_world_pos.chunk)
                {
                    Vec2_S32 test_entity_tile = tile_pos_from_chunk_rel(world, test_entity.low->world_pos.chunk_rel);
                    Vec2_S32 player_tile = tile_pos_from_chunk_rel(world, player_world_pos.chunk_rel);
                    if (test_entity_tile == player_tile) {
                        int x = 0;
                    }
                }
            }
            #endif
            
            Vec2_F32 test_entity_half_dim = 0.5f * vec2_f32(test_entity.low->width, test_entity.low->height);
            Vec2_F32 player_half_dim = 0.5f * vec2_f32(player->width, player->height);
            Vec2_F32 gjk_radius_dims = test_entity_half_dim + player_half_dim; 
    
            if (test_entity.low->is_also_high)
            {
                Vec2_F32 enitity_min = test_entity.high->camera_rel_pos - gjk_radius_dims;
                Vec2_F32 enitity_max = test_entity.high->camera_rel_pos + gjk_radius_dims; 
                F32 eps = 0.1;

                Ray_intersection_result right_wall_coll;
                {
                    F32 right_wall_main_coord = enitity_max.x + eps;
                    right_wall_coll = get_ray_intersection_result(
                        player->camera_rel.x, player->camera_rel.y,
                        displacement.x, displacement.y, 
                        right_wall_main_coord,
                        enitity_min.y, enitity_max.y
                    );
                }

                Ray_intersection_result left_wall_coll;
                {
                    F32 left_wall_main_coord = enitity_min.x - eps;
                    left_wall_coll = get_ray_intersection_result(
                        player->camera_rel.x, player->camera_rel.y,
                        displacement.x, displacement.y, 
                        left_wall_main_coord,
                        enitity_min.y, enitity_max.y
                    );
                }

                Ray_intersection_result top_wall_coll;
                {
                    F32 top_wall_main_coord = enitity_max.y + eps;
                    top_wall_coll = get_ray_intersection_result(
                        player->camera_rel.y, player->camera_rel.x,
                        displacement.y, displacement.x, 
                        top_wall_main_coord,
                        enitity_min.x, enitity_max.x
                    );
                }

                Ray_intersection_result bottom_wall_coll;
                {
                    F32 bottom_wall_main_coord = enitity_min.y - eps;
                    bottom_wall_coll = get_ray_intersection_result(
                        player->camera_rel.y, player->camera_rel.x,
                        displacement.y, displacement.x, 
                        bottom_wall_main_coord,
                        enitity_min.x, enitity_max.x
                    );
                }

                B32 collided = false;
                if (right_wall_coll.will_colide) {
                    min_time = Min(min_time, right_wall_coll.travel_time_normalized);
                    collided = true;
                }
                if (left_wall_coll.will_colide) {
                    min_time = Min(min_time, left_wall_coll.travel_time_normalized);
                    collided = true;
                }
                if (top_wall_coll.will_colide) {
                    min_time = Min(min_time, top_wall_coll.travel_time_normalized);
                    collided = true;
                }
                if (bottom_wall_coll.will_colide) {
                    min_time = Min(min_time, bottom_wall_coll.travel_time_normalized);
                    collided = true;
                }

                if (!collided) {
                    if (   (displacement.x > 0 && left_wall_coll.alredy_colliding) 
                        || (displacement.x < 0 && right_wall_coll.alredy_colliding) 
                        || (displacement.y > 0 && bottom_wall_coll.alredy_colliding) 
                        || (displacement.y < 0 && top_wall_coll.alredy_colliding)
                    ) {
                        min_time = 0.0f;
                    }
                }
                                
            }
        }
        
        Vec2_F32 corrected_displacement = displacement * min_time;
        player->camera_rel += corrected_displacement;
        if (min_time != 1.0f) {
            player->speed = vec2_f32(0.0f, 0.0f);
        }
        
        else {
            player->speed = new_speed;    
        }
        
        if (!is_camera_rel_pos_withing_camera_bounds(world, camera, player->camera_rel)) 
        {
            Assert(false);
            
            // TODO: move the camera, 
            //  calculate new camera rel for player, 
            //  update high_low entities

            // NOTE: this is some arbitrary way of moving the camera
            // TODO: make this better
            camera->world_pos.chunk_rel += (0.5f * player->camera_rel);
            recanonicalize_world_pos(world, &camera->world_pos);
    
            // NOTE: this cahanges all the entities
            change_entity_frequencies_based_on_camera_pos(world, camera);
        }

    }

}

// =========================================
// =========================================
// =========================================


// ====================================================================================






















