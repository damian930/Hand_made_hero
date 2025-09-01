#include "august.h"
#include "game_platform_comunication.h"

#include "world.cpp"

// TODO: remove the incude and chnage all the windows types inside Loaded_bitmap
#include "windows.h"
// ==============================
#if 1
#pragma pack(push, 1)
struct Some_of_the_bmp_file_header {
    // BMP file header
	WORD   FileType;     /* File type, always 4D42h ("BM") */
	DWORD  FileSize;     /* Size of the file in bytes */
	WORD   Reserved1;    /* Always 0 */
	WORD   Reserved2;    /* Always 0 */
	DWORD  BitmapOffset; /* Starting position of image data in bytes */

    // Bitmap header
    DWORD Size;            /* Size of this header in bytes */
	LONG  Width;           /* Image width in pixels */
	LONG  Height;          /* Image height in pixels */
	WORD  Planes;          /* Number of color planes */
	WORD  BitsPerPixel;    /* Number of bits per pixel */
	DWORD Compression;     /* Compression methods used */
	DWORD SizeOfBitmap;    /* Size of bitmap in bytes */
	LONG  HorzResolution;  /* Horizontal resolution in pixels per meter */
	LONG  VertResolution;  /* Vertical resolution in pixels per meter */
	DWORD ColorsUsed;      /* Number of colors in the image */
	DWORD ColorsImportant; /* Minimum number of important colors */
	/* Fields added for Windows 4.x follow this line */
	DWORD RedMask;       /* Mask identifying bits of red component */
	DWORD GreenMask;     /* Mask identifying bits of green component */
	DWORD BlueMask;      /* Mask identifying bits of blue component */
	DWORD AlphaMask;     /* Mask identifying bits of alpha component */
    // Some more bitmap header stuff
};
#pragma pack(pop)

Loaded_bitmap load_bmp_file(Arena* arena, platform_read_file_ft read_file_fp, char* bmp_file_path) 
{
    File_read_result read_result = read_file_fp(arena, bmp_file_path);
    Some_of_the_bmp_file_header* header = (Some_of_the_bmp_file_header*)read_result.data;

    Assert(header->Compression == 3);

    Loaded_bitmap result = {};
    result.pixels         = (U32*) ((U8*)read_result.data + header->BitmapOffset);
    // result.compression = ;
    result.width          = header->Width;
    result.height         = header->Height;
    result.red_mask       = header->RedMask;
    result.green_mask     = header->GreenMask;
    result.blue_mask      = header->BlueMask;
    result.alpha_mask     = header->AlphaMask;

    return result;
}

static void draw_bitmap(Loaded_bitmap loaded_bitmap, Bitmap* bitmap,
                        float start_x_f, float start_y_f,
                        float offset_x = 0.0f, float offset_y = 0.0f
) { 
    // NOTE: starts from the bottom left pixel and goes right and then up

    S32 x_cut_off = 0;
    S32 y_cut_off = 0;
    
    start_x_f -= offset_x;
    start_y_f -= offset_y;
    
    S32 min_x = (S32) start_x_f;
    S32 min_y = (S32) start_y_f;
    
    S32 max_x = min_x + loaded_bitmap.width;
    S32 max_y = min_y + loaded_bitmap.height;
    
    if (min_x < 0) {
        x_cut_off = (-1 * min_x); 
        min_x = 0;
    }
    if (min_y < 0) { 
        y_cut_off = (-1 * min_y); 
        min_y = 0;
    }
    
    if (max_x > bitmap->width) { max_x = bitmap->width; }
    if (max_y > bitmap->height) { max_y = bitmap->height; }

    U32* dest_row   = (U32*)bitmap->mem + (min_y * bitmap->width);
    U32* source_row = (U32*)loaded_bitmap.pixels + (loaded_bitmap.width * (loaded_bitmap.height - 1));
    source_row -= (y_cut_off * loaded_bitmap.width);

    for (S32 y = min_y; y < max_y; ++y) {
        U32* source_pixel = source_row;
        source_pixel += x_cut_off;
        for (S32 x = min_x; x < max_x; ++x) {
            // Find the fist set bit
            // Aplythe mask to get teh vlaue
            // Shift byt the bits calculated
            // use the value to construct a new, windws specific pixel 4-byte pixel
            U32* dest_pixel = dest_row + x;
            
            U32 red_shift;
            U32 green_shift; 
            U32 blue_shift; 
            U32 alpha_shift; 

            B32 red_found   = find_idx_of_first_least_significant_set_bit(loaded_bitmap.red_mask, &red_shift); 
            B32 green_found = find_idx_of_first_least_significant_set_bit(loaded_bitmap.green_mask, &green_shift); 
            B32 blue_found  = find_idx_of_first_least_significant_set_bit(loaded_bitmap.blue_mask, &blue_shift); 
            B32 alpha_found = find_idx_of_first_least_significant_set_bit(loaded_bitmap.alpha_mask, &alpha_shift); 

            Assert(red_found);
            Assert(green_found);
            Assert(blue_found);
            Assert(alpha_found);
            
            U32 alpha_value = ((*source_pixel & loaded_bitmap.alpha_mask) >> alpha_shift); 

            U32 s_red   = ((*source_pixel & loaded_bitmap.red_mask)   >> red_shift);
            U32 s_green = ((*source_pixel & loaded_bitmap.green_mask) >> green_shift); 
            U32 s_blue  = ((*source_pixel & loaded_bitmap.blue_mask)  >> blue_shift); 
            
            U32 d_blue  = ((*dest_pixel >> 0) & 0xFF);
            U32 d_green = ((*dest_pixel >> 8) & 0xFF);
            U32 d_red   = ((*dest_pixel >> 16) & 0xFF);

            // (1 - t)*A + t*B
            float t = (float)alpha_value / 255.0f;
            U32 new_red   = ((1 - t) * d_red) + (t * s_red);
            U32 new_green = ((1 - t) * d_green) + (t * s_green);
            U32 new_blue  = ((1 - t) * d_blue) + (t * s_blue);

            U32 new_value = (  (new_blue    << 0 ) 
                                | (new_green   << 8 )
                                | (new_red     << 16 )
                                | (alpha_value << 24 )
                               );
            
            *dest_pixel = new_value;
            source_pixel += 1;
        }
        dest_row += bitmap->width;
        source_row -= loaded_bitmap.width;
    }

}
#endif
// ===============================


file_private void draw_some_shit(Game_state* game_state, Bitmap* bitmap)
{   
    for(U32 y = 0; y < bitmap->height; y += 1) 
    {
        U32* row_start = (U32*)bitmap->mem + (y * bitmap->width); 
        for(U32 x = 0; x < bitmap->width; x += 1) 
        {
            U32* pixel = row_start + x;
            U8* byte = (U8*) pixel;
            // Blue
            *byte = 0 * y * x + (U32)game_state->background_offset + 200;
            byte += 1;
            
            // Green
            *byte = 0 * y * x + (U32)game_state->background_offset + 100;
            byte += 1;

            // Red
            *byte = (U32)(0.1f * y * x) + (U32)game_state->background_offset;
            byte += 1;

            // Padding / Alpa
            *byte = 0;
            byte += 1;
        }
    }
}

file_private void draw_black_screen(Game_state* game_state, Bitmap* bitmap)
{   
    for(U32 y = 0; y < bitmap->height; y += 1) 
    {
        U32* row_start = (U32*)bitmap->mem + (y * bitmap->width); 
        for(U32 x = 0; x < bitmap->width; x += 1) 
        {
            U32* pixel = row_start + x;
            U8* byte = (U8*) pixel;
            // Blue
            *byte = 100;
            byte += 1;
            
            // Green
            *byte = 100;
            byte += 1;

            // Red
            *byte = 100;
            byte += 1;

            // Padding / Alpa
            *byte = 0;
            byte += 1;
        }
    }
}

file_private void draw_rect(Bitmap* bitmap, 
                            F32 start_x_f, F32 start_y_f, 
                            F32 end_x_f, F32 end_y_f,
                            U8 red, U8 green, U8 blue
) {
    if (start_x_f < 0.0f) { start_x_f = 0.0f; }
    if (start_y_f < 0.0f) { start_y_f = 0.0f; }
    if (end_x_f >= (F32)bitmap->width) { end_x_f = (F32)bitmap->width; }
    if (end_y_f >= (F32)bitmap->height) { end_y_f = (F32)bitmap->height; }

    if (start_x_f >= end_x_f) { return; }
	if (start_y_f >= end_y_f) { return; }
    
    U32 start_x = (U32) start_x_f;
    U32 start_y = (U32) start_y_f;
    U32 end_x = (U32) end_x_f;  
    U32 end_y = (U32) end_y_f;  

    for(U32 y = start_y; y < end_y; y += 1) 
    {
        U32* row_of_px = bitmap->mem + (bitmap->width * y);
        for(U32 x = start_x; x < end_x; x += 1) 
        {
            U32* pixel = row_of_px + x;
            U8* byte = (U8*) pixel;
            // Blue
            *byte = blue;
            byte += 1;
            
            // Green
            *byte = green;
            byte += 1;

            // Red
            *byte = red;
            byte += 1;

            // Padding / Alpa
            *byte = 0;
            byte += 1;
        }
    }
} 

// == Working Collision for tiles ===============

B32 get_time_of_reach(F32 start_coord,
                       F32 wall_coord,
                       F32 displacement_coord,
                       F32* travel_time_normalized
) {
    if (displacement_coord == 0.0f) {
        // We cant hit the inf wall, yeah... 
        return false;        
    } 
    else 
    {
        *travel_time_normalized = (wall_coord - start_coord) / displacement_coord;
        return true;
    }
}
// ================================================

// ===============
global B32 wrote_to_file = false;

void game_update(Bitmap* bitmap, 
                 Memory* mem, 
                 Keyboard_input* keyboard, 
                 Some_more_platform_things_to_use* platform_things,
                 F32 time_elapsed
) {

#if 0
    // These are for some simple DEBUG of performance based on time
    F32 frame_start_time = platform_things->frame_start_time;
    platform_get_time_for_timing_in_sec_ft* get_time_in_sec = platform_things->get_time_for_timing_in_sec;
    Timer_since_boot_data timer_data = platform_things->timer_data;
#endif

    // Setting up the game_state when the game is first updated
    Game_state* game_state = (Game_state*)mem->permanent_mem;
    if (!game_state->is_initialised)
    {
        game_state->is_initialised = true;

        // game_state->background_offset = 161.0f;
        game_state->background_offset = 187.0f;
        // game_state->background_offset = 212.0f;
        
        game_state->px_per_m = 30;

        ///////////////////////////////////////////////////////////
        // Damian: creating the world arena
        Arena* world_arena = &game_state->arena;
        {
            *world_arena = arena_init((U8*)mem->permanent_mem + sizeof(Game_state), mem->permanent_mem_size - sizeof(Game_state));
        }

        U32 WORLD_SAFE_MARGIN_IN_CHUNKS = 50;
        U32 WORLD_START_CHUNK_POS = 100;

        ///////////////////////////////////////////////////////////
        // Damian: creating the world
        World* world = &game_state->world;
        {
            world->chunk_side_in_tiles = 16;
            world->tile_side_in_m      = 1.0f;
            world->chunk_side_in_m     = world->chunk_side_in_tiles * world->tile_side_in_m;
            
            world->chunk_safe_margin_from_middle_chunk = WORLD_SAFE_MARGIN_IN_CHUNKS;
            world->mid_chunk = vec2_s32(WORLD_START_CHUNK_POS, WORLD_START_CHUNK_POS);
            
            world->high_entity_count = 0;
        }
        
        ///////////////////////////////////////////////////////////
        // Damian: creating a simulation region for the world
        Sim_reg* sim_reg = &game_state->sim_reg;
        {
            World_pos sim_reg_world_pos = {};
            sim_reg_world_pos.chunk = vec2_s32(0, 0);
            sim_reg_world_pos.chunk_rel = 0.5f * vec2_f32(world->chunk_side_in_m);
    
            sim_reg->world_pos = sim_reg_world_pos;
            sim_reg->chunks_from_mid_chunk = 1;

            // TODO: dont like having this be hardcoded here, make a func that then can be used to do it in other places. 
            Vec2_S32 sim_reg_min_chunk = sim_reg->world_pos.chunk - vec2_s32(sim_reg->chunks_from_mid_chunk);
            Vec2_S32 sim_reg_max_chunk = sim_reg->world_pos.chunk + vec2_s32(sim_reg->chunks_from_mid_chunk);

            for (S32 chunk_y = sim_reg_min_chunk.y;
                chunk_y <= sim_reg_max_chunk.y;
                chunk_y += 1     
            ) {
                for (S32 chunk_x = sim_reg_min_chunk.x;
                    chunk_x <= sim_reg_max_chunk.x;
                    chunk_x += 1     
                ) { 
                    Chunk* chunk = get_chunk_in_world__spawns(world_arena, world, vec2_s32(chunk_x, chunk_y));

                    for (Entity_block* block_it = &chunk->entity_block;
                        block_it != 0;
                        block_it = block_it->next_block     
                    ) {
                        for (U32 low_index_inside_block = 0;
                            low_index_inside_block < block_it->low_indexes_count;
                            low_index_inside_block += 1     
                        ) {
                            // NOTE: at this point, we dont have any highs until this point, so asserting for now
                            // DATE: 1st September 2025
                            U32 low_index = block_it->low_indexes[low_index_inside_block];
                            B32 converted = convert_low_entity_to_high(world, sim_reg, low_index);
                            Assert(converted);
                            
                        }
                    }

                }
            }

        }

        ///////////////////////////////////////////////////////////
        // Damian: creating a player
        Player* player = &game_state->player;
        {
            player->sim_reg_rel = vec2_f32(0.0f, 0.0f);
            player->speed       = vec2_f32(0.0f, 0.0f);
            player->width       = 1.95f;
            player->height      = 0.55f;
            player->direction   = Entity_direction::Back;
        }

        ///////////////////////////////////////////////////////////
        // Damian: spawning a bit
        for (S32 tile_y = 0; tile_y < world->chunk_side_in_tiles; tile_y += 1)
        {
            for (S32 tile_x = 0; tile_x < world->chunk_side_in_tiles; tile_x += 1)
            {
                World_pos wall_spawn_pos = {}; 
                wall_spawn_pos.chunk = vec2_s32(0, 0);
                wall_spawn_pos.chunk_rel = world->tile_side_in_m * vec2_f32(tile_x, tile_y);

                B32 is_vert_wall = ((tile_x == 0) || (tile_x == (world->chunk_side_in_tiles - 1)));
                B32 is_hor_wall = ((tile_y == 0) || (tile_y == (world->chunk_side_in_tiles - 1)));
                B32 is_door = ((tile_x == 7) || (tile_x == 8) || (tile_x == 9) || 
                (tile_y == 7) || (tile_y == 8) || (tile_y == 9));
                
                if (tile_x == 0 && tile_y == 5) {
                    DebugStopHere;
                }

                if ((is_vert_wall || is_hor_wall) && (!is_door)) {
                    spawn_tree(&game_state->arena, world, sim_reg, 
                        wall_spawn_pos, game_state->px_per_m);
                }

            }
        }

        ///////////////////////////////////////////////////////////
        // Damian: getting some bitmaps loaded 
        game_state->player_skin_front.head = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_front_head.bmp");
        game_state->player_skin_front.cape = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_front_cape.bmp");
        game_state->player_skin_front.torso = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_front_torso.bmp");

        game_state->player_skin_back.head = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_back_head.bmp");
        game_state->player_skin_back.cape = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_back_cape.bmp");
        game_state->player_skin_back.torso = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_back_torso.bmp");

        game_state->player_skin_left.head = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_left_head.bmp");
        game_state->player_skin_left.cape = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_left_cape.bmp");
        game_state->player_skin_left.torso = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_left_torso.bmp");

        game_state->player_skin_right.head = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_right_head.bmp");
        game_state->player_skin_right.cape = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_right_cape.bmp");
        game_state->player_skin_right.torso = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_right_torso.bmp");

        game_state->player_shadow = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_hero_shadow.bmp");
        game_state->tree_00 = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\tree00.bmp");
        game_state->backdrop = load_bmp_file(&game_state->arena, platform_things->read_file_fp, "..\\art\\hand_made_01\\test_background.bmp");
    }

    // Some game_state unwrapped structs for easier referencing
    World* world = &game_state->world;
    Player* player = &game_state->player;
    Sim_reg* sim_reg = &game_state->sim_reg;

    // Moving stuff
    {
        // Getting a new direction based on the users input
        Vec2_F32 acceleration_unit_vec = {};
        if (keyboard->is_used) 
        {   
            if (keyboard->w_pressed) { acceleration_unit_vec.y = 1.0f; }
            if (keyboard->a_pressed) { acceleration_unit_vec.x = -1.0f; } 
            if (keyboard->s_pressed) { acceleration_unit_vec.y = -1.0f; } 
            if (keyboard->d_pressed) { acceleration_unit_vec.x = 1.0f; }
            if (keyboard->q_pressed) {
                player->speed = vec2_f32(0.0f, 0.0f);
            }

            if (acceleration_unit_vec.x != 0.0f && acceleration_unit_vec.y != 0.0f) {
                acceleration_unit_vec = vec2_f32(0.7071f, 0.7071f) * acceleration_unit_vec;
            }

            // Geting the direction for the player
            if (keyboard->w_pressed && 
                keyboard->a_pressed && 
                keyboard->s_pressed && 
                keyboard->d_pressed
            ) {
                player->direction = Entity_direction::Front;
            }
            else if (keyboard->w_pressed && keyboard->a_pressed) {
                player->direction = Entity_direction::Back;
            }
            else if (keyboard->w_pressed && keyboard->d_pressed) {
                player->direction = Entity_direction::Back;
            }
            else if (keyboard->s_pressed && keyboard->a_pressed) {
                player->direction = Entity_direction::Front;
            }            
            else if (keyboard->s_pressed && keyboard->d_pressed) {
                player->direction = Entity_direction::Front;
            }
            else if (keyboard->w_pressed) {
                player->direction = Entity_direction::Back;
            }
            else if (keyboard->a_pressed) {
                player->direction = Entity_direction::Left;
            } 
            else if (keyboard->s_pressed) {
                player->direction = Entity_direction::Front;
            } 
            else if (keyboard->d_pressed) {
                player->direction = Entity_direction::Right;
            }
            else {
                player->direction = Entity_direction::Front;
            }

        #if 1
            if (keyboard->r_pressed) {
                player->sim_reg_rel = vec2_f32(0.0f, 0.0f);
            }
        #endif
        
        }        

        // Getting phisics done on the player
        move_player(
            &game_state->arena, world, player, sim_reg, 
            time_elapsed,
            acceleration_unit_vec);
            // game_state->background_offset += 0.1f;
    }

    // Drawing the bitmap 
    {
        // Some constant used for drawing
        Vec2_F32 screen_offset     = vec2_f32(200.0f, 100.0f);
        World_pos player_world_pos = world_pos_from_rel_pos(world, &sim_reg->world_pos, player->sim_reg_rel);
        Vec2_F32 tree_00_offset    = vec2_f32(40.0f, 83.0f);
        
        draw_black_screen(game_state, bitmap);

        #if 1
        // Draw the map (Entities on the map)
        {   
            // TODO: draw entities better. If an entity is only partly inside a chunk, then draw the part
            //       also check neighboring chunks to see if there are entites that have parts of them on the current chunk

            World_pos chunk_to_draw_mid_pos = {};  
            chunk_to_draw_mid_pos.chunk     = player_world_pos.chunk;
            chunk_to_draw_mid_pos.chunk_rel = 0.5f * vec2_f32(world->chunk_side_in_m);
            
            // TODO: need a better name
            F32 max_distance_on_x = 0.5f * world->chunk_side_in_m + world->chunk_side_in_m;
            F32 max_distance_on_y = 0.5f * world->chunk_side_in_m + world->chunk_side_in_m;

            // NOTE: for now we expext it to alredy exist
            Chunk* chunk = get_chunk_in_world__opt(world, chunk_to_draw_mid_pos.chunk);
            Assert(chunk);

            for (Entity_block* block = &chunk->entity_block;
                    block != 0;
                    block = block->next_block
            ) {
                for (U32 block_idx = 0;
                        block_idx < block->low_indexes_count;
                        block_idx += 1 
                ) {
                    // NOTE: we expect it to be simulate when drawn for now
                    Low_entity* low = get_low_entity_from_entity_block(world, block, block_idx);
                    Assert(low->is_also_high);

                    Vec2_F32 rel_pos = sim_reg_rel_from_world_pos(world, sim_reg, &low->world_pos);
                    if (rel_pos.x < max_distance_on_x && rel_pos.y < max_distance_on_y)
                    {
                        // NOTE: this is here for assert, but not yet used, since drawing is dumb
                        High_entity* high = &world->high_entities[low->high_idx];
                    
                        Vec2_F32 entity_chunk_pos = low->world_pos.chunk_rel;
                        F32 entity_w = low->width;
                        F32 entity_h = low->height;

                        F32 entity_min_x = screen_offset.x + 
                                        (entity_chunk_pos.x * game_state->px_per_m) -
                                        (low->width / 2 * game_state->px_per_m);
                        F32 entity_max_x = entity_min_x + 
                                        (entity_w * game_state->px_per_m);

                        F32 entity_max_y = screen_offset.y + 
                                        (world->chunk_side_in_m * game_state->px_per_m) - 
                                        (entity_chunk_pos.y * game_state->px_per_m) + 
                                        (low->height / 2 * game_state->px_per_m);
                        F32 entity_min_y = entity_max_y - 
                                        (entity_h * game_state->px_per_m);

                        F32 entity_center_x = 0.5f * (entity_max_x + entity_min_x);
                        F32 entity_center_y = 0.5f * (entity_max_y + entity_min_y);

                        switch (low->type) 
                        {
                            case Entity_type::Tree: {
                                draw_bitmap(game_state->tree_00, bitmap, 
                                    entity_center_x, entity_center_y,
                                    tree_00_offset.x, tree_00_offset.y
                                );
                            } break;

                            default: {
                                InvalidCodePath;
                            } break;

                        }

                        draw_rect(bitmap, 
                            entity_min_x, entity_min_y,
                            entity_max_x, entity_max_y,
                            100, 0, 0 
                        );
                    }

                    // ===========
                    // Vec2_F32 min_pos = rel_pos - (0.5f * vec2_f32(low->width, low->height));
                    // Vec2_F32 max_pos = rel_pos + (0.5f * vec2_f32(low->width, low->height));

                }
            }

        }
        #endif
            
        // TODO: fix this, this doesnt draw if the player is outside of the box of the ga
            
        #if 0
        // Draw the tiles that player is currenly in
        {    
            World_pos player_base_min_pos = player->base_pos;
            player_base_min_pos.tile_rel -= (0.5f * vec2_f32(player->width, player->height));
            player_base_min_pos = recanonicalize_world_pos(world, player_base_min_pos);

            World_pos player_base_max_pos = player->base_pos;
            player_base_max_pos.tile_rel += (0.5f * vec2_f32(player->width, player->height));
            player_base_max_pos = recanonicalize_world_pos(world, player_base_max_pos);

            S32 min_tile_x = player->base_pos.tile.x;
            S32 min_tile_y = player->base_pos.tile.y;
            S32 max_tile_x = player->base_pos.tile.x;
            S32 max_tile_y = player->base_pos.tile.y;

            // TODO: create a func that chooses a min world position out of 2 give.
            //       store the min max positions
            //       use tile and chunk positions, subtract them from the mid point of the camera (player_base).
            //       see if these new offsets are withing the bounds of tile rectangle we draw
            //       if yes, then draw the tile, else its out of the part of the map that we draw, so draw

            if (player_base_min_pos.chunk == player->base_pos.chunk) {
                min_tile_x = Min(min_tile_x, player_base_min_pos.tile.x);
                min_tile_y = Min(min_tile_y, player_base_min_pos.tile.y);
            }
            if (player_base_max_pos.chunk == player->base_pos.chunk) {
                max_tile_x = Max(max_tile_x, player_base_max_pos.tile.x);
                max_tile_y = Max(max_tile_y, player_base_max_pos.tile.y);
            }

            for (S32 tile_x = min_tile_x; tile_x <= max_tile_x; tile_x += 1) {
                for (S32 tile_y = min_tile_y; tile_y <= max_tile_y; tile_y += 1) {
                    F32 tile_start_x = screen_offset.x + (tile_x * world->tile_side_in_m * game_state->px_per_m);
                    F32 tile_end_x   = tile_start_x + (world->tile_side_in_m * game_state->px_per_m);
                    
                    F32 tile_end_y   = screen_offset.y + (world->chunk_side_in_tiles * world->tile_side_in_m * game_state->px_per_m) - (tile_y * world->tile_side_in_m * game_state->px_per_m);
                    F32 tile_start_y = tile_end_y - (world->tile_side_in_m * game_state->px_per_m);

                    draw_rect(bitmap, 
                        tile_start_x, tile_start_y, 
                        tile_end_x, tile_end_y,
                        0, 0, 0);
                }
            }

        }
        #endif

        #if 0
        // Draw the player as as rect
        {
            F32 min_x = screen_offset.x + 
                        (player_world_pos.chunk_rel.x * game_state->px_per_m) - 
                        (player->width / 2 * game_state->px_per_m);
            F32 max_x = min_x + (player->width * game_state->px_per_m);

            F32 max_y = screen_offset.y +
                        (world->chunk_side_in_m * game_state->px_per_m) -
                        (player_world_pos.chunk_rel.y * game_state->px_per_m) + 
                        (player->height / 2 * game_state->px_per_m);
            F32 min_y = max_y - (player->height * game_state->px_per_m);

            draw_rect(bitmap, 
                min_x, min_y, 
                max_x, max_y,
                0, 200, 20);
        }
        #endif

        #if 1
        // Draw the players sprite + shadow
        {
            Vec2_F32 hero_sprite_offset = vec2_f32(72.0f, 182.0f);
            Vec2_F32 shadow_offset = hero_sprite_offset;

            F32 mid_legs_on_screen_x = screen_offset.x + 
                                    (player_world_pos.chunk_rel.x  * game_state->px_per_m);
            F32 mid_legs_on_screen_y = screen_offset.y + 
                                    (world->chunk_side_in_m * game_state->px_per_m) - 
                                    (player_world_pos.chunk_rel.y * game_state->px_per_m);

            Player_skin* skin;
            switch (game_state->player.direction) {
                case Entity_direction::Back: {
                    skin = &game_state->player_skin_back; 
                } break;

                case Entity_direction::Right: {
                    skin = &game_state->player_skin_right; 
                } break;

                case Entity_direction::Front: {
                    skin = &game_state->player_skin_front; 
                } break;

                case Entity_direction::Left: {
                    skin = &game_state->player_skin_left; 
                } break;

                default: {
                    InvalidCodePath;
                } break;
            }
            
            draw_bitmap(game_state->player_shadow, 
                bitmap, 
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                shadow_offset.x, shadow_offset.y); 
            draw_bitmap(skin->torso, 
                bitmap, 
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                hero_sprite_offset.x, hero_sprite_offset.y); 
            draw_bitmap(skin->cape, 
                bitmap, 
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                hero_sprite_offset.x, hero_sprite_offset.y);
            draw_bitmap(skin->head, 
                bitmap, 
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                hero_sprite_offset.x, hero_sprite_offset.y);
            
            draw_rect(bitmap,
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                mid_legs_on_screen_x, mid_legs_on_screen_y,
                0, 255, 0);

        }
        #endif

        #if 0
        // Draw the base point of the player
        {
            F32 player_base_on_screen_x = screen_offset.x + 
                                        (player_world_pos.chunk_rel.x * game_state->px_per_m);
            F32 player_base_on_screen_y = screen_offset.y + 
                                        (world->chunk_side_in_m * game_state->px_per_m) - 
                                        (player_world_pos.chunk_rel.y * game_state->px_per_m);
            draw_rect(bitmap, 
                player_base_on_screen_x, player_base_on_screen_y,
                player_base_on_screen_x + 3, player_base_on_screen_y + 3,
                255, 0, 0); 
        } 
        #endif
                
    }

}
















