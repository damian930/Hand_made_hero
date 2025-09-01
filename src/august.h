#ifndef AUGUST_H
#define AUGUST_H

// TODO: remove these
#include "base.h"
#include "math.h"
#include "world.h"

// These are for the game to fucntion 
struct Loaded_bitmap {
	U32* pixels;
    // uint32 compression;
    S32 width;
    S32 height;
	
    U32 red_mask;
    U32 green_mask;
    U32 blue_mask;
    U32 alpha_mask;
};
// ===========================================

struct Player {
    // NOTE: this represents the mid point of the player 
    Vec2_F32 sim_reg_rel;

    Vec2_F32 speed;
    F32 width;
    F32 height;
    Entity_direction direction;
};

struct Player_skin {
    Loaded_bitmap head;
    Loaded_bitmap cape;
    Loaded_bitmap torso;
};

struct Sim_reg {
    World_pos world_pos;     // NOTE: this is the mid chunk of the sim region
    S32 chunks_from_mid_chunk;   
};

# if 0
struct Camera_region {
    World_pos world_pos;   // NOTE: this is the middle of the camera region
    F32 width;
    F32 height;
};
#endif

struct Game_state {
    B32 is_initialised;
    F32 background_offset;

    Arena arena;

    F32 px_per_m;
    
    World world;

    Sim_reg sim_reg;
    Player player;
    
    Player_skin player_skin_front;
    Player_skin player_skin_back;
    Player_skin player_skin_left;
    Player_skin player_skin_right;

    Loaded_bitmap player_shadow;
    Loaded_bitmap tree_00;
    Loaded_bitmap backdrop;
};
// ================================================================












#endif 