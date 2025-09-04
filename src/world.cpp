#include "world.h"
#include "august.h"
#include "game_platform_comunication.h"

///////////////////////////////////////////////////////////
// Damian: Chunk stuff
//
U32 calculate_hash_for_chunk_pos(World* world, Vec2_S32 chunk_pos)
{
    // TOOD: a better hash function
    U32 hash_value = Abs(chunk_pos.x) * 73 + Abs(chunk_pos.y) * 31;
    U32 hash_slot  = hash_value & (ArrayCount(world->chunk_lists) - 1); 
    Assert(hash_slot < ArrayCount(world->chunk_lists)); 
    return hash_slot; 
}

Chunk* get_chunk_in_world__optional_spawning(World* world, Vec2_S32 chunk_pos, 
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

Chunk* get_chunk_in_world__spawns(Arena* world_arena, World* world, Vec2_S32 chunk_pos) 
{   
    Chunk* result = get_chunk_in_world__optional_spawning(world, chunk_pos, world_arena);
    Assert(result);
    return result;
}

Chunk* get_chunk_in_world__opt(World* world, Vec2_S32 chunk_pos) 
{   
    Chunk* result = get_chunk_in_world__optional_spawning(world, chunk_pos);
    return result;
}

///////////////////////////////////////////////////////////
// Damian: this was not removed on 27th of August 2025, since move_player was using this for debug purposes
//
Vec2_S32 tile_pos_from_chunk_rel(World* world, Vec2_F32 chunk_rel) {
    Vec2_S32 result = {};
    result.x = floor_F32_to_S32(chunk_rel.x / world->tile_side_in_m);
    result.y = floor_F32_to_S32(chunk_rel.y / world->tile_side_in_m);
    return result;
}

///////////////////////////////////////////////////////////
// Damian: world_pos stuff
//
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

World_pos world_pos_from_rel_pos(World* world, World_pos* world_pos, Vec2_F32 rel_pos)
{
    World_pos result = move_world_pos_by_n_meters_copy(world, world_pos, rel_pos);
    return result;
}

///////////////////////////////////////////////////////////
// Damian: sim_reg stuff
//
// --------------------------------------------------------
// TODO: see if tese type of functs are common, if yes, maybe use something like a a rectagle to store bound.
//       or just use a func that operates on a ractagle data, 
//       but then enything can be passed it, it just has to be describes as a rect
B32 is_world_pos_within_sim_reg(World* world, Sim_reg* sim_reg, World_pos test_pos)
{
    Vec2_S32 chunk_diff = test_pos.chunk - sim_reg->world_pos.chunk; 

    B32 is_valid = (   chunk_diff.x >= -sim_reg->chunks_from_mid_chunk
                    && chunk_diff.x <=  sim_reg->chunks_from_mid_chunk
                    && chunk_diff.y >= -sim_reg->chunks_from_mid_chunk
                    && chunk_diff.y <=  sim_reg->chunks_from_mid_chunk
                   );
    return is_valid;
}

B32 is_sim_reg_rel_pos_within_sim_reg_bounds(World* world, Sim_reg* sim_reg, Vec2_F32 sim_reg_rel)
{
    World_pos test_world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, sim_reg_rel);
    B32 is_within = is_world_pos_within_sim_reg(world, sim_reg, test_world_pos);
    return is_within;
}

#if 1
// NOTE: this is not a general func for the world_pos, since we need to assert here just in case
Vec2_F32 sim_reg_rel_from_world_pos(World* world, Sim_reg* sim_reg, World_pos* world_pos)
{
    Vec2_F32 rell = subtract_world_pos(world, world_pos, &sim_reg->world_pos);
    Assert(is_sim_reg_rel_pos_within_sim_reg_bounds(world, sim_reg, rell));
    return rell;
}
#endif

///////////////////////////////////////////////////////////
// Damian: Entity stuff 
//
Stored_entity* get_stored_entity(World* world, U32 stored_index) 
{
    Assert(stored_index < world->stored_entity_count);
    Stored_entity* stored = &world->stored_entities[stored_index];
    return stored;
}

// NOTE: this saves like 2 lines of code, but i spent 1.5 hours trying to fix a bug relate to this.
//       i was using a local index to a global index to acces a global low_entity
//       it would have been fine if this same error didnt happend the exact same way in 3 different places
//       (28th of August 2025 12:44 (24 hour system))
Stored_entity* get_low_entity_from_entity_block(World* world, Entity_block* block, U32 block_index)
{
    U32 stored_entity_index = block->stored_indexes[block_index];
    Stored_entity* stored = get_stored_entity(world, stored_entity_index);
    return stored;
}

void spawn_entity(Arena* world_arena, World* world, Stored_entity new_stored) 
{
    Assert(world->stored_entity_count < ArrayCount(world->stored_entities));
    Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, new_stored.world_pos.chunk);
    
    ///////////////////////////////////////////////////////////
    // Damian: Gettting a first usable block 
    //
    Entity_block* last_block;
    Entity_block* usable_block; 
    for (usable_block = &chunk->entity_block; 
         usable_block != 0;
         usable_block = usable_block->next_block
    ) {
        last_block = usable_block;
     
        if (usable_block->stored_indexes_count < ArrayCount(usable_block->stored_indexes)) {
            break;
        }

    }

    ///////////////////////////////////////////////////////////
    // Damian: Need to create a new block, since all the other once are filled 
    //
    if (!usable_block)
    {
        Entity_block* new_block = ArenaPush(world_arena, Entity_block);

        last_block->next_block = new_block;
        usable_block = new_block;
    }

    ///////////////////////////////////////////////////////////
    // Damian: Inserting the new entity 
    //
    U32 new_stored_index = world->stored_entity_count++; 
    world->stored_entities[new_stored_index] = new_stored;  

    usable_block->stored_indexes[usable_block->stored_indexes_count++] = new_stored_index;
}

void spawn_player(Arena* world_arena, World* world, World_pos player_mid_pos)
{ 
    Stored_entity new_entity = {};
    new_entity.width      = 1.95f;
    new_entity.height     = 0.55f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = player_mid_pos;
    new_entity.direction  = Entity_direction::Backward;
    new_entity.type       = Entity_type::Player;
    new_entity.has_hp     = true;
    new_entity.hp         = 3;
    new_entity.can_colide = true;

    spawn_entity(world_arena, world, new_entity);
}

void spawn_tree(Arena* world_arena, World* world, World_pos tree_mid_pos) 
{ 
    Stored_entity new_entity = {};
    new_entity.width      = 1.0f;
    new_entity.height     = 1.0f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = tree_mid_pos;
    new_entity.type       = Entity_type::Tree;
    new_entity.can_colide = true;

    spawn_entity(world_arena, world, new_entity);
}

#if 1
void spawn_familiar(Arena* world_arena, World* world,  
                    World_pos familiar_world_pos, U32 stored_entity_to_track_index 
) { 
    Stored_entity new_entity = {};
    new_entity.width  = 1.0f;
    new_entity.height = 1.0f;
    new_entity.speed  = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos = familiar_world_pos;
    new_entity.type   = Entity_type::Familiar;

    new_entity.has_entity_to_track = true;
    new_entity.stored_entity_to_track_index = stored_entity_to_track_index;

    spawn_entity(world_arena, world, new_entity);
}
#endif

void spawn_sddaword(Arena* world_arena, World* world, World_pos sword_world_pos) 
{ 
    Stored_entity new_entity = {};
    new_entity.width      = 1.0f;
    new_entity.height     = 1.0f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = sword_world_pos;
    new_entity.type       = Entity_type::Sword;
    new_entity.can_colide = true;

    spawn_entity(world_arena, world, new_entity);
}

///////////////////////////////////////////////////////////
// Damian: move the player stuff struct 
//
F32 get_ray_intersection_time(
    F32 start_main_coord, F32 start_other_coord,
    F32 displacement_main_coord, F32 displacement_other_coord, 
    F32 wall_main_coord,
    F32 wall_other_min_coord, F32 wall_other_max_coord
) {
    F32 result_time = 1.0;

    if (displacement_main_coord != 0.0f) 
    {   
        F32 time_to_hit_an_inf_wall_at_main_coord = (wall_main_coord - start_main_coord) / displacement_main_coord;
        // if (time_to_hit_an_inf_wall_at_main_coord == -0.0f) {
        //     time_to_hit_an_inf_wall_at_main_coord = 0.0f;
        // }
        if (time_to_hit_an_inf_wall_at_main_coord >= 0.0f)
        {
            F32 possible_displacement_other_coord = (time_to_hit_an_inf_wall_at_main_coord * displacement_other_coord); 
            F32 wall_other_coord = start_other_coord + possible_displacement_other_coord;
            
            if (   wall_other_coord >= wall_other_min_coord
                && wall_other_coord <= wall_other_max_coord
            ) {
                result_time = time_to_hit_an_inf_wall_at_main_coord;
                
                F32 time_eps = 0.01;
                result_time -= time_eps;
            }
        }
    }
    
    return result_time;
}

// TODO: move these parametes into a more coherent struct
//       then store some data inside the entities when spawning that relates the update policy for their type
struct Entity_movement_data {
    // TODO: see if passing a non unit vector is more comfortable
    F32 time_elapsed;
    Vec2_F32 acceleration_unit_vec;
    F32 acceleration_mult;
};
void move_entity(World* world, Sim_reg* sim_reg, 
                 U32 sim_entity_index,         
                 Entity_movement_data* movement_data
) {
    Sim_entity* sim_entity = &sim_reg->sim_entities[sim_entity_index];

    if (sim_entity->stored_variant.type == Entity_type::Player)
    {
        if ((U32)sim_entity->stored_variant.direction != 0) {
            DebugStopHere;
        }
    } 

    ///////////////////////////////////////////////////////////
    // Damian: Calculating movement data
    //
    // --------------------------------------------------------
    // TODO: see if acceleration_mult is something that should just be stored inside the stored entity
    F32 t                 = movement_data->time_elapsed;
    F32 acceleration_mult = movement_data->acceleration_mult;
    Vec2_F32 a            = movement_data->acceleration_unit_vec * acceleration_mult; 
    Vec2_F32 new_speed    = sim_entity->stored_variant.speed + (a * t);
    Vec2_F32 displacement = (0.5f * a * t*t) + (sim_entity->stored_variant.speed * t);
    // NOTE: this is something like friction but a very vanila hardcoded version
    new_speed /= 1.5f; 

    ///////////////////////////////////////////////////////////
    // Damian: Doing collision detection
    //
    if (displacement != vec2_f32(0.0f, 0.0f)) 
    {
        Vec2_S32 min_high_chunk = sim_reg->world_pos.chunk - vec2_s32(sim_reg->chunks_from_mid_chunk);
        Vec2_S32 max_high_chunk = sim_reg->world_pos.chunk + vec2_s32(sim_reg->chunks_from_mid_chunk);
        
        ///////////////////////////////////////////////////////////
        // Damian: We are trying to get these values from the following loop
        F32 min_time = 1.0f;
        Vec2_S32 wall_hit_direction_unit = {};

        if (sim_entity->stored_variant.can_colide)
        {
            // TODO: dont forget that there might be entites outside of the sim region, 
            //       acount those for collision

            for (U32 sim_reg_index = 0; 
                sim_reg_index < sim_reg->sim_entitie_count;
                sim_reg_index += 1
            ) {
                if (sim_entity_index == sim_reg_index) {
                    continue;
                }

                Sim_entity* other_sim_entity = &sim_reg->sim_entities[sim_reg_index];
                if (other_sim_entity->stored_variant.can_colide) 
                {
                    F32 GJK_width  = sim_entity->stored_variant.width + other_sim_entity->stored_variant.width;
                    F32 GJK_height = sim_entity->stored_variant.height + other_sim_entity->stored_variant.height;

                    Vec2_F32 enitity_min = -0.5f * vec2_f32(GJK_width, GJK_height); 
                    Vec2_F32 enitity_max = 0.5f * vec2_f32(GJK_width, GJK_height);  
                    
                    Vec2_F32 test_entity_to_moved_entity = 
                        sim_entity->sim_reg_rel - other_sim_entity->sim_reg_rel; 
            
                    F32 eps = 0.1;
        
                    F32 right_wall_coll_time = get_ray_intersection_time(
                            test_entity_to_moved_entity.x, test_entity_to_moved_entity.y,
                            displacement.x, displacement.y, 
                            enitity_max.x,
                            enitity_min.y, enitity_max.y
                        );

                    F32 left_wall_coll_time = get_ray_intersection_time(
                            test_entity_to_moved_entity.x, test_entity_to_moved_entity.y,
                            displacement.x, displacement.y, 
                            enitity_min.x,
                            enitity_min.y, enitity_max.y
                        );
        
                    F32 top_wall_coll_time = get_ray_intersection_time(
                            test_entity_to_moved_entity.y, test_entity_to_moved_entity.x,
                            displacement.y, displacement.x, 
                            enitity_max.y,
                            enitity_min.x, enitity_max.x
                        );
        
                    F32 bottom_wall_coll_time = get_ray_intersection_time(
                            test_entity_to_moved_entity.y, test_entity_to_moved_entity.x,
                            displacement.y, displacement.x, 
                            enitity_min.y,
                            enitity_min.x, enitity_max.x
                        );
                    
                    {
                        F32 min_time_old = min_time;
                        min_time = Min(min_time, right_wall_coll_time);
                        min_time = Min(min_time, left_wall_coll_time);
                        if (min_time < min_time_old) {
                            if (displacement.y > 0.0f) {
                                wall_hit_direction_unit = vec2_s32(0, 1);
                            }
                            else if (displacement.y < 0.0f) {
                                wall_hit_direction_unit = vec2_s32(0, -1);
                            }
                        }
                    }

                    {
                        F32 min_time_old = min_time;
                        min_time = Min(min_time, top_wall_coll_time);
                        min_time = Min(min_time, bottom_wall_coll_time);
                        if (min_time < min_time_old) {
                            if (displacement.x > 0.0f) {
                                wall_hit_direction_unit = vec2_s32(1, 0);
                            }
                            else if (displacement.x < 0.0f) {
                                wall_hit_direction_unit = vec2_s32(-1, 0);
                            }
                        }
                    }

                }
                
            }
            
        }
        ///////////////////////////////////////////////////////////
        // Damian: moving the player, adjusting the speed vector when colision
        //
        Vec2_F32 corrected_displacement = displacement * min_time;
        sim_entity->sim_reg_rel += corrected_displacement;
        if (min_time != 1.0f) 
        {
            sim_entity->stored_variant.speed = vec2_f32(0.0f);

            #if 0
            Vec2_F32 displacement_after_collision = displacement - corrected_displacement; 
            Vec2_F32 wall_unit_vec_as_f32 = {};
            wall_unit_vec_as_f32.x = (F32)wall_hit_direction_unit.x;
            wall_unit_vec_as_f32.y = (F32)wall_hit_direction_unit.y;
            
            Vec2_F32 projected_onto_the_wall = vec2_f32_dot(displacement_after_collision, wall_unit_vec_as_f32) * wall_unit_vec_as_f32; 
            // Vec2_F32 projected_as_unit_vec = vec2_f32_unit(projected_onto_the_wall);
            Vec2_F32 projected_with_same_length = wall_unit_vec_as_f32 * vec2_f32_len(displacement_after_collision);  

            // low->speed = projected_with_same_length * low->speed * 20;

            Vec2_F32 temp = vec2_f32_dot(low->speed, wall_unit_vec_as_f32) * wall_unit_vec_as_f32;
            low->speed = (wall_unit_vec_as_f32 * low->speed) - temp;
            // Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
            #endif
        }
        else {
            sim_entity->stored_variant.speed = new_speed;    
        }
    }

}

// TODO: see if using indexes is better when refering to the simulated intities
//       maybe use both (pointer + index), SOME FOOD FOR THOUGHT

///////////////////////////////////////////////////////////
// Damian: more entity stuff, particularly update functions
//
void update_player(World* world, Sim_reg* sim_reg, 
                   U32 sim_entity_index, 
                   Keyboard_input* keyboard,
                   F32 time_elapsed
) {
    Sim_entity* sim_entity = &sim_reg->sim_entities[sim_entity_index];

    ///////////////////////////////////////////////////////////
    // Damian: Processing keyboard input and preparing data for moving the player
    //
    Vec2_F32 acceleration_unit_vec = {};
    if (keyboard->is_used) 
    {   
        if (keyboard->w_pressed) { acceleration_unit_vec.y = 1.0f; }
        if (keyboard->a_pressed) { acceleration_unit_vec.x = -1.0f; } 
        if (keyboard->s_pressed) { acceleration_unit_vec.y = -1.0f; } 
        if (keyboard->d_pressed) { acceleration_unit_vec.x = 1.0f; }
        if (keyboard->q_pressed) {
            sim_reg->sim_entities[sim_entity_index].stored_variant.speed = vec2_f32(0.0f, 0.0f); 
        }

        F32 diagonal_unit_len = 0.7071f;
        if (acceleration_unit_vec.x != 0.0f && acceleration_unit_vec.y != 0.0f) {
            acceleration_unit_vec = vec2_f32(diagonal_unit_len) * acceleration_unit_vec;
        }

        ///////////////////////////////////////////////////////////
        // Damian: Acceleration based direction
        Vec2_F32 forward_vec        = vec2_f32(0.0f, 1.0f);
        Vec2_F32 forward_right_vec  = vec2_f32(diagonal_unit_len, diagonal_unit_len);
        
        Vec2_F32 right_vec          = vec2_f32(1.0f, 0.0f);
        Vec2_F32 backward_right_vec = vec2_f32(diagonal_unit_len, -diagonal_unit_len);
        
        Vec2_F32 backward_vec       = vec2_f32(0.0f, -1.0f);
        Vec2_F32 backward_left_vec  = vec2_f32(-diagonal_unit_len, -diagonal_unit_len);
        
        Vec2_F32 left_vec           = vec2_f32(-1.0f, 0.0f);
        Vec2_F32 forward_left_vec   = vec2_f32(-diagonal_unit_len, diagonal_unit_len);

        Entity_direction* player_direction = &sim_entity->stored_variant.direction;
        if (acceleration_unit_vec == forward_vec) {
            *player_direction = Entity_direction::Forward;
        }            
        else if (acceleration_unit_vec == forward_right_vec) {
            *player_direction = Entity_direction::Forward;
        }
        else if (acceleration_unit_vec == right_vec) {
            *player_direction = Entity_direction::Right;
        }
        else if (acceleration_unit_vec == backward_right_vec) {
            *player_direction = Entity_direction::Backward;
        }
        else if (acceleration_unit_vec == backward_vec) {
            *player_direction = Entity_direction::Backward;
        }
        else if (acceleration_unit_vec == backward_left_vec) {
            *player_direction = Entity_direction::Backward;
        }
        else if (acceleration_unit_vec == left_vec) {
            *player_direction = Entity_direction::Left;
        }
        else if (acceleration_unit_vec == forward_left_vec) {
            *player_direction = Entity_direction::Forward;
        }
        else {
            InvalidCodePath;
        }

        #if 1
            if (keyboard->r_pressed) {
                sim_reg->sim_entities[sim_entity_index].sim_reg_rel = vec2_f32(0.0f, 0.0f);
            }
        #endif

        #if 0
            if (keyboard->left_arrow_pressed) {
                World_pos player_world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, player_high->sim_reg_rel);
                World_pos sword_world_pos = player_world_pos;
                sword_world_pos.chunk_rel += vec2_f32(3.0f, 0.0f);
                spawn_sword(game_state, sword_world_pos);
            }
        #endif
    }
  
    
    Entity_movement_data player_move_data = {};
    player_move_data.time_elapsed          = time_elapsed;
    player_move_data.acceleration_unit_vec = acceleration_unit_vec;
    player_move_data.acceleration_mult     = 225;

    move_entity(world, sim_reg, 
                sim_entity_index,
                 &player_move_data);

    DebugStopHere;
}

#if 1
void update_familiar(World* world, Sim_reg* sim_reg, 
                     U32 sim_entity_index, F32 time_elapsed
) {
    Sim_entity* sim_familiar = &sim_reg->sim_entities[sim_entity_index]; 
    Assert(sim_familiar->stored_variant.type == Entity_type::Familiar);
    Assert(sim_familiar->stored_variant.has_entity_to_track);

    F32 max_distance = 10.0f;
    F32 min_distance = 3.0f;
    F32 acceleration_mult = 75;

    Stored_to_sim_entity_ref* ref_opt = 
        get_stored_entity_ref_inside_sim_reg__opt(sim_reg, sim_familiar->stored_variant.stored_entity_to_track_index);
    
    // NOTE: we expect the entity that the familiar is tracking to be references in the current sim_reg
    Assert(ref_opt); 
    
    Vec2_F32 diff;
    if (ref_opt->is_sim)
    {
        Sim_entity* sim_tracked_entity = &sim_reg->sim_entities[ref_opt->sim_index]; 
        diff = sim_tracked_entity->sim_reg_rel - sim_familiar->sim_reg_rel; 
    }
    else 
    {   
        Stored_entity* stored_tracked_entity = &world->stored_entities[ref_opt->stored_index];
        
        World_pos* tracked_entity_world_pos = &stored_tracked_entity->world_pos;
        World_pos sim_familiar_world_pos = 
            world_pos_from_rel_pos(world, &sim_reg->world_pos, sim_familiar->sim_reg_rel);

        diff = subtract_world_pos(world, tracked_entity_world_pos, &sim_familiar_world_pos);
    }

    if (vec2_f32_len_squared(diff) >= Square(min_distance) &&
        vec2_f32_len_squared(diff) <= Square(max_distance)
    ) {
        Entity_movement_data familiar_movement_data = {};
        familiar_movement_data.time_elapsed          = time_elapsed;
        familiar_movement_data.acceleration_unit_vec = diff / vec2_f32_len(diff);
        familiar_movement_data.acceleration_mult     = 100;

        move_entity(world, sim_reg, sim_entity_index, &familiar_movement_data);
    }
    DebugStopHere;
}
#endif

///////////////////////////////////////////////////////////
// Damian: Stored/Simulated entity stuff for the sim region(s)
//

// IDEAS: start a simulation at a point
//        do stuff with it 
//        end simulation 

Sim_reg* start_sim_reg(Arena* world_arena, Arena* frame_arena, 
                       World* world, 
                       World_pos sim_world_pos, S32 chunks_from_mid_chunk)
{
    Sim_reg* new_sim_reg = ArenaPush(frame_arena, Sim_reg);
    new_sim_reg->world_pos             = sim_world_pos;
    new_sim_reg->chunks_from_mid_chunk = chunks_from_mid_chunk;

    new_sim_reg->sim_entitie_count = 0;

    Vec2_S32 min_sim_chunk = sim_world_pos.chunk - vec2_s32(chunks_from_mid_chunk);
    Vec2_S32 max_sim_chunk = sim_world_pos.chunk + vec2_s32(chunks_from_mid_chunk);

    for (S32 chunk_y = min_sim_chunk.y; 
         chunk_y <= max_sim_chunk.y; 
         chunk_y += 1)
    {
        for (S32 chunk_x = min_sim_chunk.x; 
             chunk_x <= max_sim_chunk.x; 
             chunk_x += 1)
        {
            // TODO: figure out what to do with spawning here
            // TODO: Need to spawn less for the non player sim regions i guess
            Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, vec2_s32(chunk_x, chunk_y));
            Assert(chunk);

            for (Entity_block* block = &chunk->entity_block; 
                 block != 0; 
                 block = block->next_block)
            {
                for (U32 block_stored_index = 0; 
                     block_stored_index < block->stored_indexes_count; 
                     block_stored_index += 1) 
                {
                    U32 world_stored_index = block->stored_indexes[block_stored_index];
                    Stored_entity* stored = &world->stored_entities[world_stored_index];

                    ///////////////////////////////////////////////////////////
                    // Damian: Converting stored to simulated
                    //
                    Assert(new_sim_reg->sim_entitie_count < ArrayCount(new_sim_reg->sim_entities));
                    
                    if (is_world_pos_within_sim_reg(world, new_sim_reg, stored->world_pos)) 
                    {
                        if (stored->has_entity_to_track) 
                        {
                            add_stored_entity_ref_to_sim_reg_if_not_there(frame_arena, 
                                new_sim_reg, 
                                stored->stored_entity_to_track_index);
                        }
                        
                        Sim_entity* new_sim_entity = &new_sim_reg->sim_entities[new_sim_reg->sim_entitie_count];
                        new_sim_reg->sim_entitie_count += 1;                        
                        
                        new_sim_entity->sim_reg_rel    = sim_reg_rel_from_world_pos(world, new_sim_reg, &stored->world_pos);
                        new_sim_entity->stored_variant = *stored;
                        new_sim_entity->stored_index   = world_stored_index;
                    }

                }
            }

        }
    }

    for (U32 sim_entity_index = 0; 
         sim_entity_index < new_sim_reg->sim_entitie_count; 
         sim_entity_index += 1
    ) {
        Sim_entity* sim = &new_sim_reg->sim_entities[sim_entity_index];

        Stored_to_sim_entity_ref* opt_ref = get_stored_entity_ref_inside_sim_reg__opt(new_sim_reg, sim->stored_index);
        if (opt_ref)
        {
            // NOTE: This mean a sim entity is referenced, need to flag it
            opt_ref->is_sim    = true;
            opt_ref->sim_index = sim_entity_index;
            Assert(opt_ref->stored_index == sim->stored_index);
        }
    
    }

    return new_sim_reg;
}

// TODO: Deal with the fact that sim_reg was allocated on the arena when in was started
void end_sim_reg(World* world, Sim_reg* sim_reg)
{
    for (U32 sim_index = 0; sim_index < sim_reg->sim_entitie_count; sim_index += 1) 
    {
        Sim_entity* sim = &sim_reg->sim_entities[sim_index];
     
        if (sim->stored_variant.type == Entity_type::Player)
        {
            if ((U32)sim->stored_variant.direction != 0) {
                DebugStopHere;
            }
        } 

        Stored_entity new_stored = {};
        new_stored = sim->stored_variant;
        new_stored.world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, sim->sim_reg_rel); 
        
        // TODO: chage this, dont like the fact that we store index to low inside the simulated entity state
        world->stored_entities[sim->stored_index] = new_stored;
    }
    
    sim_reg->sim_entitie_count = 0;
    // TODO: Deal with frame_arena here
    // ...
}














