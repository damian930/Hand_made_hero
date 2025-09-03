#include "world.h"
#include "august.h"

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

Vec2_F32 sim_reg_rel_from_world_pos(World* world, Sim_reg* sim_reg, World_pos* world_pos)
{
    Vec2_F32 rell = subtract_world_pos(world, world_pos, &sim_reg->world_pos);
    Assert(is_sim_reg_rel_pos_within_sim_reg_bounds(world, sim_reg, rell));
    return rell;
}

///////////////////////////////////////////////////////////
// Damian: Entity stuff 
//
Low_entity* get_low_entity(World* world, U32 low_index) 
{
    Assert(low_index < world->low_entity_count);
    Low_entity* low = &world->low_entities[low_index];
    return low;
}

// NOTE: this saves like 2 lines of code, but i spent 1.5 hours trying to fix a bug relate to this.
//       i was using a local index to a global index to acces a global low_entity
//       it would have been fine if this same error didnt happend the exact same way in 3 different places
//       (28th of August 2025 12:44 (24 hour system))
Low_entity* get_low_entity_from_entity_block(World* world, Entity_block* block, U32 block_index)
{
    U32 low_entity_index = block->low_indexes[block_index];
    Low_entity* low = get_low_entity(world, low_entity_index);
    return low;
}

B32 convert_low_entity_to_high(World* world, Sim_reg* sim_reg, U32 low_index) 
{
    Assert(world->high_entity_count < ArrayCount(world->high_entities));
    
    Low_entity* low = get_low_entity(world, low_index);
    B32 did_convert = false;
    
    if (!low->is_also_high && 
        is_world_pos_within_sim_reg(world, sim_reg, low->world_pos)
    ) {
        did_convert = true;

        U32 new_high_index = world->high_entity_count++; 
        low->is_also_high = true;
        low->high_idx = new_high_index;

        High_entity* new_high = &world->high_entities[new_high_index];
        new_high->sim_reg_rel = sim_reg_rel_from_world_pos(world, sim_reg, &low->world_pos);
        new_high->direction   = Entity_direction::Back;
        new_high->low_index   = low_index;
    }

    return did_convert;
}

B32 convert_high_entity_to_low(World* world, Sim_reg* sim_reg, U32 low_index)
{
    Low_entity* low = get_low_entity(world, low_index);
    
    if (!low->is_also_high) {
        return false;
    }

    High_entity* high = &world->high_entities[low->high_idx];

    low->world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, high->sim_reg_rel);

    U32 last_high_index = world->high_entity_count - 1;
    U32 high_index_to_delete = low->high_idx;
    
    if (high_index_to_delete != last_high_index)
    {
        High_entity* high_to_delete = &world->high_entities[low->high_idx]; 
        High_entity* last_high = &world->high_entities[last_high_index];
        *high_to_delete = *last_high;
        world->low_entities[last_high->low_index].high_idx = high_index_to_delete;
    }
    world->high_entity_count -= 1;
    low->is_also_high = false;

    return true;
}

#if 0
file_private
void unload_chunk(World* world, Vec2_S32 chunk_pos)
{
    Chunk* chunk = get_chunk_in_world__opt(world, chunk_pos);

    // NOTE: Since we are unloading the chunk, it should alredy be created
    Assert(chunk);

    for (Entity_block* block_it = &chunk->entity_block;
            block_it != 0;
            block_it = block_it->next_block     
    ) {
        for (U32 low_index_inside_block = 0;
                low_index_inside_block < block_it->low_indexes_count;
                low_index_inside_block += 1     
        ) {
            U32 low_index = block_it->low_indexes[low_index_inside_block];
            Low_entity* low = get_low_entity(world, low_index);

            // NOTE: Since we are unloading, entity inside this chunk, shoud be high
            Assert(low->is_also_high);

            convert_high_entity_to_low(world, low_index);
        }
    }
}

file_private
void load_chunk(Arena* world_arena, World* world, Camera_* camera, Vec2_S32 chunk_pos)
{
    Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, chunk_pos);

    for (Entity_block* block_it = &chunk->entity_block;
            block_it != 0;
            block_it = block_it->next_block     
    ) {
        for (U32 low_index_inside_block = 0;
                low_index_inside_block < block_it->low_indexes_count;
                low_index_inside_block += 1     
        ) {
            U32 low_index = block_it->low_indexes[low_index_inside_block];
            Low_entity* low = get_low_entity(world, low_index);

            // NOTE: Since we are loading, entity inside this chunk cant alredy be high
            Assert(!low->is_also_high);

            convert_low_entity_to_high(world, camera, low_index);
        }
    }
}
#endif

void spawn_entity(Game_state* game_state, Low_entity new_low_entity) 
{
    Arena* world_arena = &game_state->arena;
    World* world       = &game_state->world;
    Sim_reg* sim_reg   = &game_state->sim_reg;

    Assert(world->low_entity_count < ArrayCount(world->low_entities));
    Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, new_low_entity.world_pos.chunk);
    
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
    
    *new_low = new_low_entity;
    Assert(!new_low_entity.is_also_high);
    new_low->is_also_high = false;

    usable_block->low_indexes[usable_block->low_indexes_count++] = new_low_index;
    B32 converted = convert_low_entity_to_high(world, sim_reg, new_low_index);
    Assert(converted);
}

// TODO: maybe just pass in game state, since it takes all the parts of it in
void spawn_player(Game_state* game_state, World_pos player_mid_pos)
{ 
    Low_entity new_entity = {};
    new_entity.width      = 1.95f;
    new_entity.height     = 0.55f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = player_mid_pos;
    new_entity.type       = Entity_type::Player;
    new_entity.has_hp     = true;
    new_entity.hp         = 3;
    new_entity.can_colide = true;

    spawn_entity(game_state, new_entity);
}

void spawn_tree(Game_state* game_state, World_pos tree_mid_pos) 
{ 
    Low_entity new_entity = {};
    new_entity.width      = 1.0f;
    new_entity.height     = 1.0f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = tree_mid_pos;
    new_entity.type       = Entity_type::Tree;
    new_entity.can_colide = true;

    spawn_entity(game_state, new_entity);
}

void spawn_familiar(Game_state* game_state,  
                    World_pos familiar_world_pos, U32 high_entity_to_track_index 
) { 
    Low_entity new_entity = {};
    new_entity.width  = 1.0f;
    new_entity.height = 1.0f;
    new_entity.speed  = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos = familiar_world_pos;
    new_entity.type   = Entity_type::Familiar;
    
    new_entity.has_entity_to_track = true;
    new_entity.high_entity_to_track_index = high_entity_to_track_index;

    spawn_entity(game_state, new_entity);
}

void spawn_sword(Game_state* game_state, World_pos sword_world_pos) 
{ 
    if (game_state->does_sword_exist) {
        return;
    }
    game_state->does_sword_exist = true;

    Low_entity new_entity = {};
    new_entity.width      = 1.0f;
    new_entity.height     = 1.0f;
    new_entity.speed      = vec2_f32(0.0f, 0.0f);
    new_entity.world_pos  = sword_world_pos;
    new_entity.type       = Entity_type::Sword;
    new_entity.can_colide = true;

    spawn_entity(game_state, new_entity);
}

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

// NOTE: This is the main function that takes care of World simulation region
//       and camera relative both for the player and the entities
B32 fix_sim_reg_world_invariant(Arena* world_arena,
                               World* world, 
                               Sim_reg* sim_reg 
) {
    Assert(validate_low_high_entity_pairs(world));
    
    Low_entity* player_low = get_low_entity(world, world->player_low_index);
    Assert(player_low->is_also_high);
    High_entity* player_high = &world->high_entities[player_low->high_idx];

    World_pos player_world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, player_high->sim_reg_rel);
    if (player_world_pos.chunk == sim_reg->world_pos.chunk) {
        return false;
    }

    ///////////////////////////////////////////////////////////
    // Damian: have to move entites from high to low
    //
    // NOTE: since we are about to move the sim region placement,
    //       we have to convert current high into low, under the current sim_reg regime 
    #define this_is_a_part_of_not_having_branes
    this_is_a_part_of_not_having_branes {
        Assert(world->low_entities[world->high_entities[0].low_index].type == Entity_type::Player);
    
        for (U32 high_entity_index = 0;
             high_entity_index < world->high_entity_count;
             high_entity_index += 0
        ) {
            High_entity* high = &world->high_entities[high_entity_index];

            if (high->low_index != world->player_low_index)
            {
                Low_entity* low = &world->low_entities[high->low_index];

                if (low->type == Entity_type::Familiar) {
                    DebugStopHere;
                }

                B32 converted = convert_high_entity_to_low(world, sim_reg, high->low_index);
                if (converted) {
                    // NOTE: convert_high_entity_to_low will do the += 1 for the iteration,
                    //       its kinda not great this way
                } 
                else {
                    InvalidCodePath;
                }
            }
            else {
                high_entity_index += 1;
            }

        }
    }
    // =========================================

    ///////////////////////////////////////////////////////////
    // Damian: move the camera
    //
    sim_reg->world_pos.chunk = player_world_pos.chunk;
    sim_reg->world_pos.chunk_rel = 0.5f * vec2_f32(world->chunk_side_in_m);
    
    ///////////////////////////////////////////////////////////
    // Damian: change the player rel
    // 
    Vec2_F32 new_player_rel = sim_reg_rel_from_world_pos(world, sim_reg, &player_world_pos);
    player_high->sim_reg_rel = new_player_rel;
    Assert(is_sim_reg_rel_pos_within_sim_reg_bounds(world, sim_reg, player_high->sim_reg_rel));

    Vec2_F32 correct_player_rel = player_high->sim_reg_rel;

    ///////////////////////////////////////////////////////////
    // Damian: spawn chunks that are to be simualated, 
    //         redo high entities
    // 
    Vec2_S32 min_high_chunk = sim_reg->world_pos.chunk - vec2_s32(sim_reg->chunks_from_mid_chunk);
    Vec2_S32 max_high_chunk = sim_reg->world_pos.chunk + vec2_s32(sim_reg->chunks_from_mid_chunk);

    // NOTE: This is the current version of moving low to high and high into low when camera moved
    // TODO: Do it like you have funcking brains

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
                        if (low->type != Entity_type::Player) {
                            low->is_also_high = false;
                            
                            B32 converted = convert_low_entity_to_high(world, sim_reg, low_index);
                            Assert(converted);
                        }
                        else {
                            DebugStopHere;
                        }
                    }

                    if (correct_player_rel != player_high->sim_reg_rel) {
                        DebugStopHere;
                    }

                }
            }

        }
    }

    DebugStopHere;
    Assert(validate_low_high_entity_pairs(world));

    #undef this_is_a_part_of_not_having_branes

    return true;
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

void move_entity(Game_state* game_state,
                 U32 low_entity_index, 
                 F32 time_elapsed, Vec2_F32 acceleration_unit_vec, F32 acceleration_mult
) {
    Arena* world_arena = &game_state->arena;
    World* world = &game_state->world;
    Sim_reg* sim_reg = &game_state->sim_reg;

    Low_entity* low = get_low_entity(world, low_entity_index);
    Assert(low->is_also_high);
    High_entity* high = &world->high_entities[low->high_idx];

    F32 t = time_elapsed;

    Vec2_F32 a = acceleration_unit_vec * acceleration_mult;
    Vec2_F32 new_speed = low->speed + (a * t);
    Vec2_F32 displacement = (a/2 * t*t) + (low->speed * t);
    new_speed /= 1.5f; // NOTE: this is something like friction but a very vanila hardcoded version

    if (displacement != vec2_f32(0.0f, 0.0f)) 
    {
        Vec2_S32 min_high_chunk = sim_reg->world_pos.chunk - vec2_s32(sim_reg->chunks_from_mid_chunk);
        Vec2_S32 max_high_chunk = sim_reg->world_pos.chunk + vec2_s32(sim_reg->chunks_from_mid_chunk);
        
        // We are trying to get these values in the following loop
        F32 min_time = 1.0f;
        Vec2_S32 wall_hit_direction_unit = {};

        if (low->can_colide)
        {
            for (S32 high_chunk_y = min_high_chunk.y;
                high_chunk_y <= max_high_chunk.y;
                high_chunk_y += 1     
            ) {
                for (S32 high_chunk_x = min_high_chunk.x;
                        high_chunk_x <= max_high_chunk.x;
                        high_chunk_x += 1     
                ) {
                    Chunk* chunk = get_chunk_in_world__opt(world, vec2_s32(high_chunk_x, high_chunk_y));
                    Assert(chunk) // NOTE: it has to be created , sine we should be able to be moving outside the sim reg for now, and when we have sim reg, all the things insifde of it are supposed to alredy exist
                    Entity_block* test_first_block = &chunk->entity_block;

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
        
                            if (test_high->low_index == high->low_index) {
                                continue;
                            }

                            if (test_low->type == Entity_type::Tree) {
                                DebugStopHere;
                            }

                            if (test_low->can_colide) 
                            {
                                F32 GJK_width  = test_low->width + low->width;
                                F32 GJK_height = test_low->height + low->height;

                                Vec2_F32 enitity_min = -0.5f * vec2_f32(GJK_width, GJK_height); 
                                Vec2_F32 enitity_max = 0.5f * vec2_f32(GJK_width, GJK_height);  
                                
                                Vec2_F32 test_entity_to_moved_entity = high->sim_reg_rel - test_high->sim_reg_rel;
                        
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

                }
            }

        }

        ///////////////////////////////////////////////////////////
        // Damian: moving the player, adjusting the speed vector when colision
        //
        Vec2_F32 corrected_displacement = displacement * min_time;
        high->sim_reg_rel += corrected_displacement;
        if (min_time != 1.0f) 
        {

            low->speed = vec2_f32(0.0f);

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
            low->speed = new_speed;    
        }
        
        fix_sim_reg_world_invariant(world_arena, world, sim_reg);
    }

}

///////////////////////////////////////////////////////////
// Damian: more entity stuff, particularly update functions
//
// TODO: see if i would make more sence to store low and high and indexes togethew, 
//       so less accessing is then required
void update_familiar(Game_state* game_state, 
                     U32 familiar_low_index, F32 time_elapsed
) {
    World* world = &game_state->world;

    Low_entity* familiar_low   = &world->low_entities[familiar_low_index];
    Assert(familiar_low->type == Entity_type::Familiar);
    Assert(familiar_low->is_also_high);
    High_entity* familiar_high = &world->high_entities[familiar_low->high_idx];

    F32 max_distance = 10.0f;
    F32 min_distance = 3.0f;
    F32 acceleration_mult = 75;

    High_entity* followed_high = &world->high_entities[familiar_low->high_entity_to_track_index];
    Vec2_F32 diff = followed_high->sim_reg_rel - familiar_high->sim_reg_rel;
    
    if (vec2_f32_len_squared(diff) >= Square(min_distance) &&
        vec2_f32_len_squared(diff) <= Square(max_distance)
    ) {
        Vec2_F32 acc_unit_vec = diff / vec2_f32_len(diff);
        move_entity(game_state, familiar_low_index, time_elapsed, acc_unit_vec, acceleration_mult);
    }
    DebugStopHere;

}




















