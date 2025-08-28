#include "world.h"
#include "august.h"

file_private 
U32 calculate_hash_for_chunk_pos(World* world, Vec2_S32 chunk_pos)
{
    // TOOD: a better hash function
    U32 hash_value = Abs(chunk_pos.x) * 73 + Abs(chunk_pos.y) * 31;
    U32 hash_slot  = hash_value & (ArrayCount(world->chunk_lists) - 1); 
    Assert(hash_slot < ArrayCount(world->chunk_lists)); 
    return hash_slot; 
}

// TODO: dont spawn every time, have a separate that can return a chuink without spawning it
//       this should grant more controll over the allocation(creation) of the world

file_private 
Chunk* get_chunk_in_world__optional_spawning(World* world, 
                                  Vec2_S32 chunk_pos, 
                                  Arena* world_arena = 0
) {
    Chunk* result = 0;
    
    U32 hash = calculate_hash_for_chunk_pos(world, chunk_pos);
    Chunk_list* chunk_list = &world->chunk_lists[hash];
    
    if (chunk_list->is_initialised)
    {
        Assert(chunk_list->chunk);
        Chunk* last_chunk = chunk_list->chunk;
        for (Chunk* it_chunk = chunk_list->chunk; 
            it_chunk != 0; 
            it_chunk = it_chunk->next_in_chunk_list
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
            if (world_arena)
            {
                // TODO: make this better
                Chunk* new_chunk = ArenaPush(world_arena, Chunk);
                new_chunk->next_in_chunk_list = 0;
                new_chunk->chunk_pos = chunk_pos;
    
                last_chunk->next_in_chunk_list = new_chunk;
                result = new_chunk;
            }
        }
    }
    else
    {   
        if (world_arena)
        {
            chunk_list->is_initialised = true;
            chunk_list->chunk = ArenaPush(world_arena, Chunk); 
            chunk_list->chunk->next_in_chunk_list = 0;
            chunk_list->chunk->chunk_pos = chunk_pos;
            chunk_list->chunk->entity_block = {};
            
            result = chunk_list->chunk;
        }
    }

    return result;
} 

file_private 
Chunk* get_chunk_in_world__spawns(Arena* world_arena, World* world, Vec2_S32 chunk_pos) 
{   
    Chunk* result = get_chunk_in_world__optional_spawning(world, chunk_pos, world_arena);
    Assert(result);
    return result;
}

file_private
Chunk* get_chunk_in_world__opt(World* world, Vec2_S32 chunk_pos) 
{   
    Chunk* result = get_chunk_in_world__optional_spawning(world, chunk_pos);
    return result;
}

// ====================================================================================

// NOTE: this was not removed on 27th of August 2025, since move_player was using this for debug purposes
file_private
Vec2_S32 tile_pos_from_chunk_rel(World* world, Vec2_F32 chunk_rel) {
    Vec2_S32 result = {};
    result.x = floor_F32_to_S32(chunk_rel.x / world->tile_side_in_m);
    result.y = floor_F32_to_S32(chunk_rel.y / world->tile_side_in_m);
    return result;
}

file_private
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
        DebugStopHere;
    }
    return is_valid;
}

file_private
void recanonicalize_world_pos(World* world, World_pos* invalid_pos) {
    S32 chunk_offset_x = floor_F32_to_S32(invalid_pos->chunk_rel.x / world->chunk_side_in_m);
    S32 chunk_offset_y = floor_F32_to_S32(invalid_pos->chunk_rel.y / world->chunk_side_in_m);

    invalid_pos->chunk_rel.x -= chunk_offset_x * world->chunk_side_in_m;
    invalid_pos->chunk_rel.y -= chunk_offset_y * world->chunk_side_in_m;

    invalid_pos->chunk.x += chunk_offset_x;
    invalid_pos->chunk.y += chunk_offset_y;
    
    Assert(is_world_pos_canonical(world, *invalid_pos));
};

file_private
World_pos recanonicalize_world_pos_copy(World* world, World_pos* invalid_pos) {
    World_pos result = *invalid_pos;
    recanonicalize_world_pos(world, &result);
    return result;
};

file_private
void move_world_pos_by_n_meters(World* world, World_pos* pos, Vec2_F32 m) {
    pos->chunk_rel += m;
    recanonicalize_world_pos(world, pos);
}

file_private
World_pos move_world_pos_by_n_meters_copy(World* world, World_pos* pos, Vec2_F32 m) {
    World_pos result = *pos;
    move_world_pos_by_n_meters(world, &result, m);
    return result;
}

file_private
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

file_private 
Low_entity* get_low_entity(World* world, U32 low_index) 
{
    Assert(low_index < world->low_entity_count);

    Low_entity* low = &world->low_entities[low_index];
    return low;
}

// TODO: i dont like that this says asserts
//       maybe just use the _opt version and then asserm manually, cause this assert is more for debug,
//       so having it as a separate func is kinda dumb maybe
file_private
Entity_block* get_entity_block__asserts(World* world, Vec2_S32 chunk_pos) {
    Chunk* chunk = get_chunk_in_world__opt(world, chunk_pos);
    Assert(chunk);
    return &chunk->entity_block;
}

// NOTE: this saves like 2 lines of code, but i spent 1.5 hours trying to fix a bug relate to this.
//       i was using a local index to a global index to acces a global low_entity
//       it would have been fine if this same error didnt happend the exact same way in 3 different places
//       (28th of August 2025 12:44 (24 hour system))
file_private
Low_entity* get_low_entity_from_entity_block(World* world, Entity_block* block, U32 block_index)
{
    U32 low_entity_index = block->low_indexes[block_index];
    Low_entity* low = get_low_entity(world, low_entity_index);
    return low;
}

file_private
B32 convert_low_entity_to_high_if_needed(World* world, Camera* camera, U32 low_index
) {
    Assert(world->high_entity_count < ArrayCount(world->high_entities));
    Low_entity* low = get_low_entity(world, low_index);
    B32 did_convert = false;
    
    if (!low->is_also_high && 
        is_world_pos_within_camera_bounds(world, camera, low->world_pos)
    ) {
        did_convert = true;

        if (low->world_pos.chunk_rel.x != 0.0f && low->world_pos.chunk_rel.y != 0.0f) {
            DebugStopHere;
        }

        U32 new_high_index = world->high_entity_count++; 
        low->is_also_high = true;
        low->high_idx = new_high_index;

        High_entity* new_high = &world->high_entities[new_high_index];
        new_high->camera_rel_pos = camera_rel_pos_from_world_pos(world, camera, low->world_pos);
        new_high->direction      = Entity_direction::Back;
        new_high->low_index      = low_index;
    }

    return did_convert;
}

file_private
void convert_low_entity_to_high(World* world, Camera* camera, U32 low_index)
{
    B32 did_convert = convert_low_entity_to_high_if_needed(world, camera, low_index);
    if (!did_convert) {
        InvalidCodePath;
    }
}

// TODO: think about a function like "spawn_tree_maybe" that only spawn if the chunk exists
//       right now this creates a world chunk if it doesnt exist
//       i am fine with it, just dont like that we might forget about it, 
//       since the name encapsulates this piece of logic too much

// TODO: this is fine, we jsut need a way to profile the game itslef, 
//       wether it spawns some things too far out of reach

file_private
void spawn_entity(Arena* world_arena, World* world, Camera* camera,
                  World_pos entt_mid_pos, 
                  F32 entt_width, F32 entt_height, 
                  Entity_type entt_type,
                  Vec2_F32 entt_speed
) {
    Assert(world->low_entity_count < ArrayCount(world->low_entities));
    Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, entt_mid_pos.chunk);
    
    // Gettting a first usable block
    Entity_block* last_block;
    Entity_block* usable_block; 
    for (usable_block = &chunk->entity_block; 
         usable_block != 0;
         usable_block = usable_block->next_block
    ) {
        last_block = usable_block;
     
        if (usable_block->low_indexes_count < ArrayCount(usable_block->low_indexes)) {
            break;
        }

    }

    // Need to create a new block, since all the other once are filled
    if (!usable_block)
    {
        Entity_block* new_block = ArenaPush(world_arena, Entity_block);

        last_block->next_block = new_block;
        usable_block = new_block;
    }

    // Inserting the new entity 
    U32 new_low_index = world->low_entity_count++; 
    Low_entity* new_low = &world->low_entities[new_low_index];    
    new_low->type         = entt_type;
    new_low->world_pos    = entt_mid_pos; // NOTE: this is the pos of the middle of an entity
    new_low->speed        = entt_speed;
    new_low->width        = entt_width;
    new_low->height       = entt_height;
    new_low->is_also_high = false;

    usable_block->low_indexes[usable_block->low_indexes_count++] = new_low_index;
    convert_low_entity_to_high_if_needed(world, camera, new_low_index);
}

file_private
void spawn_tree(Arena* world_arena, World* world, Camera* camera, 
                World_pos tree_mid_pos, F32 px_per_m
) { 
    // TODO: maybe this is not the way to do it
    F32 tree_width_in_px  = 75.0f - 7.0f;
    F32 tree_height_in_px = 105.0f;

    F32 width        = 1.0f;
    F32 height       = 1.0f;
    Vec2_F32 speed   = vec2_f32(0.0f, 0.0f);
    Entity_type type = Entity_type::Tree;

    spawn_entity(world_arena, world, camera, 
                 tree_mid_pos, width, height, 
                 type, speed);

}

file_private
B32 validate_low_high_entity_pairs(World* world)
{
    B32 is_valid = true;
    for (U32 low_index = 0; low_index < world->low_entity_count; low_index += 1)
    {
        Low_entity* low = get_low_entity(world, low_index);
        if (low->is_also_high)
        {
            is_valid = (world->high_entities[low->high_idx].low_index == low_index); 
            if (!is_valid) {
                break;
            }
        }
    }
    return is_valid;
}

#if 0
file_private
void convert_low_entities_inside_camera_region_into_high_if_needed(
    World* world, Camera* camera
) {
    Vec2_S32 min_high_chunk = camera->world_pos.chunk - vec2_s32(camera->chunks_from_camera_chunk);
    Vec2_S32 max_high_chunk = camera->world_pos.chunk + vec2_s32(camera->chunks_from_camera_chunk);

    for (S32 high_chunk_y = min_high_chunk.y;
         high_chunk_y <= max_high_chunk.y;
         high_chunk_y += 1     
    ) {
        for (S32 high_chunk_x = min_high_chunk.x;
             high_chunk_x <= max_high_chunk.x;
             high_chunk_x += 1     
        ) { 
            Entity_block* test_first_block = get_entity_block__asserts(world, vec2_s32(high_chunk_x, high_chunk_y)); 
                    
            for (Entity_block* block_it = test_first_block;
                 block_it != 0;
                 block_it = block_it->next_block     
            ) {
                for (U32 low_idx = 0;
                     low_idx < block_it->low_entity_count;
                     low_idx += 1     
                ) {
                    Low_entity* low = &block_it->low_entities[low_idx];
                    convert_low_entity_to_high_if_needed(world, camera, low);
                }
            }
        }
    }

}
#endif

// NOTE: This is the main function that takes care of World simulation region
//       and camera relative both for the player and the entities
file_private
void fix_camera_world_invariant(Arena* world_arena,
                                World* world, 
                                Camera* camera, 
                                Player* player
) {
    Assert(validate_low_high_entity_pairs(world));

    // 1. Move the camera
    World_pos player_world_pos = world_pos_from_camera_rel(world, camera, player->camera_rel);
    camera->world_pos = player_world_pos;
    
    // 2. Change the player_rel 
    // NOTE: this vec2_f32(0.0f) world if we set the camera to be the current players pos
    //       otherwise we would need to adjust the player better, since just 0.0f would move the player
    {
        Assert(player_world_pos.chunk == camera->world_pos.chunk);
        Assert(player_world_pos.chunk_rel == camera->world_pos.chunk_rel);
        // NOTE: if this fails, then the camera is using a more sofisticated movemened, 
        //       so change the player adjustment losic and remove these Asserts
    }
    Vec2_F32 new_player_camera_rel = vec2_f32(0.0f);
    player->camera_rel = new_player_camera_rel;
    Assert(is_camera_rel_pos_withing_camera_bounds(world, camera, player->camera_rel));

    // 3 + 4. 
    // Spawn chunks that are to be simualated    
    // Redo high entities
    Vec2_S32 min_high_chunk = camera->world_pos.chunk - vec2_s32(camera->chunks_from_camera_chunk);
    Vec2_S32 max_high_chunk = camera->world_pos.chunk + vec2_s32(camera->chunks_from_camera_chunk);

    // NOTE: This is the current version of moving low to high and high into low when camera moved
    // TODO: Do it like you have funcking brains
    #define this_is_a_part_of_not_having_branes
    this_is_a_part_of_not_having_branes world->high_entity_count = 0;
    // =========================================

    for (S32 high_chunk_y = min_high_chunk.y;
         high_chunk_y <= max_high_chunk.y;
         high_chunk_y += 1     
    ) {
        for (S32 high_chunk_x = min_high_chunk.x;
             high_chunk_x <= max_high_chunk.x;
             high_chunk_x += 1     
        ) { 
            Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, vec2_s32(high_chunk_x, high_chunk_y));

            for (Entity_block* block_it = &chunk->entity_block;
                 block_it != 0;
                 block_it = block_it->next_block     
            ) {
                for (U32 low_index_inside_block = 0;
                     low_index_inside_block < block_it->low_indexes_count;
                     low_index_inside_block += 1     
                ) {
                    // TODO: Need to get newly high into high
                    //       Need to get the newly low out of high
                    U32 low_index = block_it->low_indexes[low_index_inside_block];
                    Low_entity* low = get_low_entity(world, low_index);

                    this_is_a_part_of_not_having_branes {
                        low->is_also_high = false;
                    }
                    convert_low_entity_to_high_if_needed(world, camera, low_index);
                }
            }

        }
    }

    DebugStopHere;
    Assert(validate_low_high_entity_pairs(world));

    #undef this_is_a_part_of_not_having_branes

}

file_private
void fix_camera_world_invariant_if_needed(Arena* world_arena,
                                          World* world, 
                                          Camera* camera, 
                                          Player* player
) {
    World_pos player_world_pos = world_pos_from_camera_rel(world, camera, player->camera_rel);
    if (player_world_pos.chunk != camera->world_pos.chunk)
    {
        fix_camera_world_invariant(world_arena, world, camera, player);
    }

}

// =========================================
// =========================================
// =========================================
// =========================================

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

void move_player(Arena* world_arena,
                 World* world, 
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
        // TODO: see if this type thing is used else wher, and make a func out of it called 
        //       "get camera rect bound in world chunks"
        Vec2_S32 min_high_chunk = camera->world_pos.chunk - vec2_s32(camera->chunks_from_camera_chunk);
        Vec2_S32 max_high_chunk = camera->world_pos.chunk + vec2_s32(camera->chunks_from_camera_chunk);

        // We are trying to get these values in the following loop
        F32 min_time = 1.0f;
        Vec2_S32 wall_hit_direction = {};

        for (S32 high_chunk_y = min_high_chunk.y;
             high_chunk_y <= max_high_chunk.y;
             high_chunk_y += 1     
            ) {
                for (S32 high_chunk_x = min_high_chunk.x;
                     high_chunk_x <= max_high_chunk.x;
                     high_chunk_x += 1     
                ) { 
                    Entity_block* test_first_block = get_entity_block__asserts(world, vec2_s32(high_chunk_x, high_chunk_y)); 
                    
                    for (Entity_block* block_it = test_first_block;
                         block_it != 0;
                         block_it = block_it->next_block     
                    ) {
                        for (U32 low_idx = 0;
                             low_idx < block_it->low_indexes_count;
                             low_idx += 1     
                        ) { 
                            Low_entity* test_low = get_low_entity_from_entity_block(world, block_it, low_idx);
                            Assert(test_low->is_also_high);
                            High_entity* test_high = &world->high_entities[test_low->high_idx];
                            
                            #if 1
                            {
                                World_pos player_world_pos = world_pos_from_camera_rel(world, camera, player->camera_rel);
                                if (test_low->world_pos.chunk == player_world_pos.chunk)
                                {
                                    Vec2_S32 test_entity_tile = tile_pos_from_chunk_rel(world, test_low->world_pos.chunk_rel);
                                    Vec2_S32 player_tile = tile_pos_from_chunk_rel(world, player_world_pos.chunk_rel);
                                    if (test_entity_tile == player_tile) {
                                        DebugStopHere;
                                    }
                                }
                            }
                            #endif
                            
                            Vec2_F32 test_entity_half_dim = 0.5f * vec2_f32(test_low->width, test_low->height);
                            Vec2_F32 player_half_dim = 0.5f * vec2_f32(player->width, player->height);
                            Vec2_F32 gjk_radius_dims = test_entity_half_dim + player_half_dim; 
                    
                            Vec2_F32 enitity_min = test_high->camera_rel_pos - gjk_radius_dims;
                            Vec2_F32 enitity_max = test_high->camera_rel_pos + gjk_radius_dims; 
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
        
        fix_camera_world_invariant_if_needed(world_arena, world, camera, player);
        
    }

}

// =========================================
// =========================================
// =========================================


// ====================================================================================






















