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
    Front,
    Right,
    Back,
    Left,
};

namespaced_enum Entity_update_frequency : U32 {
    Low,
    Low_High,
};

namespaced_enum Entity_type : U32 {
    Tree,
};

struct Low_entity {
    B32 is_also_high;
    U32 high_idx;

    Entity_type type;
    World_pos world_pos; // NOTE: this is the pos of the middle of an entity
    Vec2_F32 speed;
    F32 width;
    F32 height;
};

// NOTE: these are camera_pos relative 
struct High_entity {
    U32 low_index;

    Vec2_F32 camera_rel_pos; // NOTE: this is the position of the middle of an entity
    Entity_direction direction;
};

#if 0
struct Entity {
    Low_entity* low;
    High_entity* high;
};
#endif

struct Entity_block {
    Low_entity low_entities[4]; // TODO: make this value better (the size of the array)
    U32 low_entity_count;
    
    Entity_block* next_block;
};

struct Chunk {
    Chunk* next_in_hash;

    Vec2_S32 chunk_pos;
    Entity_block entity_block;
};

struct Chunk_list {
    // NOTE: This boolean here is just to know if data has not yet been initialised
    // TODO: THink of a better scheme for this, i dont like a boolean here
    B32 is_initialised;
    Chunk* chunk;
};

// THINGS_TO_IMPLEMENT: 
// -- Some way to reference then from outer position isnide the world

struct World {
    U32 chunk_side_in_tiles;
    F32 tile_side_in_m;
    F32 chunk_side_in_m;
    
    S32 chunk_safe_margin_from_middle_chunk;
    Vec2_S32 mid_chunk;

    Chunk_list chunk_lists[1024];
    
    High_entity high_entities[128]; 
    U32 high_entity_count;
};








#endif 



