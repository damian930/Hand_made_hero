#ifndef WORLD_H
#define WORLD_H

#include "base.h"
#include "math.h"

struct World_pos {
    Vec2_S32 chunk;      // NOTE: this is from the middle chunk of the map
    Vec2_F32 chunk_rel;  // NOTE: this is from the lower-left corner of a chunk
};

namespaced_enum Entity_direction : U32 { 
    Forward = 1,
    Right,
    Backward,
    Left,
};

namespaced_enum Entity_type : U32 {
    Player = 1,
    Tree,
    Familiar,
    Sword,
};

struct Stored_entity {
    Entity_type type;
    World_pos world_pos; // NOTE: this is the pos of the middle of an entity
    Vec2_F32 speed;
    F32 width;
    F32 height;

    Entity_direction direction;

    B32 can_colide;

    B32 has_hp;
    U32 hp;

    B32 has_entity_to_track;
    U32 stored_entity_to_track_index;
};

struct Sim_entity {
    Vec2_F32 sim_reg_rel;

    Stored_entity stored_variant;
    U32 stored_index;
    // NOTE: this is just here now, since this is not the final version of this
    //       everythingfromthe stored is used, excpet for the world_pos,
    //       since it is the property of the Stored and not the Simulated
};

///////////////////////////////////////////////////////////
// Damian: Sim_reg + some hash_thigs needed for referencisng simulated entities
//
struct Stored_to_sim_entity_ref {
    U32 stored_index;

    B32 is_sim;
    U32 sim_index;

    Stored_to_sim_entity_ref* next;
};

struct Stored_to_sim_entity_ref_list {
    B32 is_initialised;
    Stored_to_sim_entity_ref* ref;
};

struct Sim_reg {
    World_pos world_pos;       // NOTE: this is the mid chunk of the sim region
    S32 chunks_from_mid_chunk;

    Sim_entity sim_entities[1024];
    U32 sim_entitie_count;

    Stored_to_sim_entity_ref_list entity_ref_list[3];
};

U32 calculate_hash_entity_ref(Sim_reg* sim_reg, U32 stored_entity_to_reference_index)
{
    // TOOD: a better hash function
    U32 hash_slot = stored_entity_to_reference_index & (ArrayCount(sim_reg->entity_ref_list) - 1); 
    Assert(hash_slot < ArrayCount(sim_reg->entity_ref_list)); 
    return hash_slot;
}

Stored_to_sim_entity_ref* get_stored_entity_ref_inside_sim_reg__opt(Sim_reg* sim_reg, 
                                                                    U32 stored_entity_to_reference_index
) {
    U32 hash = calculate_hash_entity_ref(sim_reg, stored_entity_to_reference_index);
    Stored_to_sim_entity_ref_list* ref_list = &sim_reg->entity_ref_list[hash];  

    Stored_to_sim_entity_ref* result = 0;
    
    if (ref_list->is_initialised)
    {   
        for (Stored_to_sim_entity_ref* ref = ref_list->ref; ref != 0; ref = ref->next)
        {
            if (ref->stored_index == stored_entity_to_reference_index)
            {
                result = ref;
                break;
            }
        }
        return result;
    }
    else {
        return result;
    }

}

void add_stored_entity_ref_to_sim_reg_if_not_there(Arena* arena, 
                                                   Sim_reg* sim_reg, 
                                                   U32 stored_entity_to_reference_index
) {
    if(!get_stored_entity_ref_inside_sim_reg__opt(sim_reg, stored_entity_to_reference_index))
    {
        U32 hash = calculate_hash_entity_ref(sim_reg, stored_entity_to_reference_index);
        Stored_to_sim_entity_ref_list* ref_list = &sim_reg->entity_ref_list[hash];  

        B32 just_created = false;
        if (!ref_list->is_initialised) {
            ref_list->ref = ArenaPush(arena, Stored_to_sim_entity_ref);
            ref_list->is_initialised = true;
            just_created = true;
        }

        if (just_created)
        {
            // NOTE: the first ref consists of uninitialised data (zero-ed out) 
            ref_list->ref->stored_index = stored_entity_to_reference_index;
            ref_list->ref->is_sim       = false;
            ref_list->ref->sim_index    = 0;
            ref_list->ref->next         = NULL;
        }
        else 
        {
            Stored_to_sim_entity_ref* last_visited_ref = 0;
            for (Stored_to_sim_entity_ref* last_visited_ref = ref_list->ref;
                 last_visited_ref->next != 0;
                 last_visited_ref = last_visited_ref->next
            ) { }
            last_visited_ref->next = ArenaPush(arena, Stored_to_sim_entity_ref);
            last_visited_ref->next->stored_index = stored_entity_to_reference_index;
            last_visited_ref->next->is_sim       = false;
            last_visited_ref->next->sim_index    = 0;
            last_visited_ref->next->next         = NULL;
        }
    }

}

// --------------------------------------------------------

struct Entity_block {
    U32 stored_indexes[4];    // TODO: make this value better (the size of the array)
    U32 stored_indexes_count;
    
    Entity_block* next_block;
};

struct Chunk {
    Chunk* next_in_chunk_list;

    Vec2_S32 chunk_pos;
    Entity_block entity_block;
};

struct Chunk_list {
    // NOTE: This boolean here is just to know if data has not yet been initialised
    // TODO: Think of a better scheme for this, i dont like a boolean here
    B32 is_initialised;
    Chunk* chunk;
};

struct World {
    U32 chunk_side_in_tiles;
    F32 tile_side_in_m;
    F32 chunk_side_in_m;
    
    S32 chunk_safe_margin_from_middle_chunk;
    Vec2_S32 mid_chunk;
    
    Stored_entity stored_entities[1024];
    U32 stored_entity_count;
    
    U32 player_stored_index;

    Chunk_list chunk_lists[1024];
};


#endif 



