#ifndef WORLD_H
#define WORLD_H

#include "base.h"
#include "math.h"

// TODO: Dont know how i feel about this.
// This was dont to not have B32 is_initialised inside Chunk,
// cause then every chunk stores it and also the first chunk ib hash list if the only one that is not null but also not initialised.
// Storing a struct that then has the list itself of chunks might be better, 
// but the current version of it i dont like

// TODO: think about doing this from the middle of a chunk
struct World_pos {
    Vec2_S32 chunk;      // NOTE: this is from the middle chunk of the map
    Vec2_F32 chunk_rel;  // NOTE: this is from the lower-left corner of a chunk
};

namespaced_enum Entity_direction : U32 { 
    Front = 1,
    Right,
    Back,
    Left,
};

namespaced_enum Entity_type : U32 {
    Player = 1,
    Tree,
    Familiar,
    Sword,
};

#if 0
struct __Low_entity {
    B32 is_also_high;
    U32 high_idx;

    Entity_type type;
    World_pos world_pos; // NOTE: this is the pos of the middle of an entity
    Vec2_F32 speed;
    F32 width;
    F32 height;

    B32 can_colide;

    B32 has_hp;
    U32 hp;

    B32 has_entity_to_track;
    U32 high_entity_to_track_index;
};

// NOTE: these are camera_pos relative 
struct __High_entity {
    U32 low_index;

    Vec2_F32 sim_reg_rel;  // NOTE: this is the position of the middle of an entity
    Entity_direction direction;
};
#endif

struct Stored_entity {
    Entity_type type;
    World_pos world_pos; // NOTE: this is the pos of the middle of an entity
    Vec2_F32 speed;
    F32 width;
    F32 height;

    B32 can_colide;

    B32 has_hp;
    U32 hp;

    B32 has_entity_to_track;
    U32 high_entity_to_track_index;
};

struct Sim_entity {
    Vec2_F32 sim_reg_rel;

    Stored_entity stored_variant;
    U32 stored_index;
    // NOTE: this is just here now, since this is not the final version of this
    //       everythingfromthe stored is used, excpet for the world_pos,
    //       since it is the property of the Stored and not the Simulated
};

struct Sim_reg {
    World_pos world_pos;     // NOTE: this is the mid chunk of the sim region
    S32 chunks_from_mid_chunk;

    Sim_entity sim_entities[1024];
    U32 sim_entitie_count;
};

struct Entity_block {
    U32 stored_indexes[4]; // TODO: make this value better (the size of the array)
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
    // TODO: THink of a better scheme for this, i dont like a boolean here
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



