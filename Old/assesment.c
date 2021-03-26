#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <cab202_graphics.h>
#include <cab202_sprites.h>
#include <cab202_timers.h>
#include <stdio.h>
#include <time.h>


#define DELAY (10)  
#define PLAYER_WIDTH (3)
#define PLAYER_HEIGHT (4)
#define MAX_BLOCK (1000000)
sprite_id zombies[MAX_BLOCK];
int num_blocks = 10;

int spawn_line[200];
bool game_over = false; /* Set this to true when game is over */
bool update_screen = true; /* Set to false to prevent screen update. */
bool pause_treasure = false;
int elapsed_time;
int x_spawn;
int y_spawn;
double start_time;
bool paused = false;
double lives = 10;
double score = 0;
int safe_block_count = 1;
int death_block_count = 0;
int max_safe_blocks = 0;
int num_zombies = 180;
int runtime = 0;
int run_treasure = 0;
int run_time_1 = 0;
int o = 0;
bool gravity = true; 
int gravofftime = 0;
bool playerjump = false;
int maxrow1 = 0;

char * player_respawn =
/**/	"\\\\ | //"
/**/	"-- * --"
/**/	"-- * --"
/**/	"// | \\\\";

char * player_image =
/**/	" O "
/**/	"/|\\"
/**/	" | "
/**/	"/ \\";

char * player_image_jump =
/**/	" O "
/**/	"\\|/"
/**/	" | "
/**/	"\\ /";

char * player_image_left =
/**/	" O "
/**/	"\\|\\"
/**/	" | "
/**/	"\\ \\";

char * player_image_right =
/**/	" O "
/**/	"/|/"
/**/	" | "
/**/	"/ /";

char * block_image =
/**/	"======="
/**/	"=======";

char * block_death_image =
/**/	"XXXXXXX"
/**/	"XXXXXXX";

char * treasure_image =
/**/	"$^"
/**/	"V$";

char * treasure_image_2 =
/**/	"V$"
/**/	"$^";

char * player_current_img = 
/**/	" O "
/**/	"/|\\"
/**/	" | "
/**/	"/ \\";

sprite_id player;
sprite_id player_jump;
sprite_id player_left;
sprite_id player_right;
sprite_id spawn_block_image;
sprite_id safe_block[180];
sprite_id death_block[40];
sprite_id treasure[1];
sprite_id respawn;


double timer(void) {
    elapsed_time = (int) (get_current_time() - start_time);
    draw_formatted(36,2,"%02d:%02d", elapsed_time/60, elapsed_time%60);
    return elapsed_time;
}



int read_char() {
    int key_code = paused ? wait_char() : get_char();
    if ('p' == key_code) {
        paused = !paused;
    }
    return key_code;
}

void draw_hud() {
    int screenwidth = screen_width();
    draw_line(0, 0, screenwidth, 0, '~');
    draw_line(0, 4, screenwidth, 4, '~');
    draw_string(1, 2, "n10224378");
    draw_string(15, 2, "Lives:");
    draw_string(30, 2, "Time:");
    timer();
    draw_double(55, 20, time, FG_COLOUR);
    draw_string(45, 2, "Score:");
    draw_double(52,2, score);
    draw_double(22,2, lives);

}

sprite_id create_safe_block(int c, int y) {
    sprite_id new_block = sprite_create(c, y, 7, 2, block_image);
    max_safe_blocks += 1;
    return new_block;
}

sprite_id create_death_block(int c, int y) {
    sprite_id new_block = sprite_create(c, y, 7, 2, block_death_image);
    max_safe_blocks += 1;
    return new_block;
}



void block_direction(sprite_id block_type[], int block_count, int run_time_1) {
         if (run_time_1 == 0) {
             if (run_time_1 == 0 && block_type == safe_block) {
                 maxrow1 += 1;
             }
            sprite_turn_to(block_type[block_count], 0.1, 0.0);
            sprite_turn(block_type[block_count], 0);
            
        }
        if (run_time_1 == 1) {
            sprite_turn_to(block_type[block_count], -0.1, 0.0);
            sprite_turn(block_type[block_count], 0);
        }
        if (run_time_1 == 2) {
            sprite_turn_to(block_type[block_count], 0.1, 0.0);
            sprite_turn(block_type[block_count], 0);
            
        }
}

void level_setup(void) {
    int screenwidth = screen_width();
    int screenheight = screen_height();
    int y = 10;
    int l = 0;
    int x_value;
    run_time_1 = 0;
    srand(time(NULL));
    while (l <= (screenheight - 7)){
        for (int c = 0; c < screenwidth - 14; c += 8) {
            spawn_line[runtime] = rand() % 10;
            x_value = spawn_line[runtime];
        if (safe_block_count != 160) {
            if (x_value <= 5) {
                safe_block[safe_block_count] = create_safe_block(c, y);
                block_direction(safe_block, safe_block_count, run_time_1);
                safe_block_count += 1;
                }   
            }
        if (death_block_count != 40) {
            if (x_value <= 8 && x_value >= 7 ) {
                death_block[death_block_count] = create_death_block(c, y);
                block_direction(death_block, death_block_count, run_time_1);
                death_block_count += 1;
            }
        }
            runtime += 1;
            x_value = 0;
        }
        if (run_time_1 == 2) {
            run_time_1 = 0;
        }
        run_time_1 += 1;
    y += 7;
    l += 6;  
    }
}

bool sprites_collide(sprite_id s1, sprite_id s2)
{
    if ((!s1->is_visible) || (!s2->is_visible)) {
        return false;
    }
    int l1 = round(sprite_x(s1));
    int l2 = round(sprite_x(s2));
    int t1 = round(sprite_y(s1));
    int t2 = round(sprite_y(s2));
    int r1 = l1 + sprite_width(s1);
    int r2 = l2 + sprite_width(s2) - 1;
    int b1 = t1 + sprite_height(s1);
    int b2 = t2 + sprite_height(s2) -1;

    if (l1 > r2)
        return false;
    if (l2 > r1)
        return false;
    if (t1 > b2)
        return false;
    if (t2 > b1)
        return false;
    return true;
}

sprite_id sprites_collide_any(sprite_id s, sprite_id sprites[], int n)
{
    sprite_id result = NULL;

    for (int i = 0; i < n; i++)
    {
        if (sprites_collide(s, sprites[i]))
        {
            result = sprites[i];
        }
    }

    return result;
}

void auto_move(sprite_id sid, int keyCode)
{
    if (!sid->is_visible)
    {
        return;
    }
    if (keyCode < 0) {
        sprite_step(sid);
        int px = round(sprite_x(sid));
        double pdx = sprite_dx(sid);
        if (px == 0 || px == screen_width() - 1) {
            pdx = -pdx;
        }
        if (pdx != sprite_dx(sid)) {
            sprite_back(sid);
            sprite_turn_to(sid, pdx, 0);

        }
    }
}

void treasure_animation(void) {
    if (o == 0) {
        sprite_set_image(treasure[0], treasure_image_2);
        o += 1;
    }
    else {
        sprite_set_image(treasure[0], treasure_image);
        o = 0;
    }
}

void auto_move_platform(sprite_id sid, int keyCode, char * sprite_image)
{
    if (!sid->is_visible)
    {
        return;
    }
    if (keyCode < 0) {
        sprite_step(sid);
        int px = round(sprite_x(sid));
        int py = round(sprite_y(sid));
        double pdx = sprite_dx(sid);
        // /double pdy = sprite_dy(sid);
        if (px == 0) {
            pdx = screen_width() - 2;
        }
        else if (px == screen_width() - 1) {
            pdx = 2; 
        }
        if (pdx != sprite_dx(sid)) {
            sid->x = pdx;
            sid = sprite_create(pdx, py, sprite_width(sid), sprite_height(sid), sprite_image);
            sprite_turn_to(sid, 0.1, 0);
            treasure_animation();
        }

    }
}


void move_player(int key){
    int hx = round(sprite_x(player));
    int hy = round(sprite_y(player));
    if (gravity != false) {
        if (key < 0) {
            sprite_set_image(player, player_image);
            player_current_img = player_image;
        }
        if (key == 'a' && hx > 1) {
            sprite_move(player, -1, 0);
            sprite_set_image(player, player_image_left);
            player_current_img = player_image_left;
        }
        else if (key == 'd' && hx < (screen_width() - 1 - sprite_width(player))) {
            sprite_move(player, +1, 0);
            sprite_set_image(player, player_image_right);
            player_current_img = player_image_right;
        }
        else if (key == 'w' && hy > 5) {
            sprite_move(player, 0, -1);
            gravity = false;
            playerjump = true;
            sprite_set_image(player, player_image_jump);
            player_current_img = player_image_jump;
        }
    }
}

void setup(void) {
    level_setup();
    draw_hud();
    player = sprite_create((screen_width() - 6), 6, 3, 4, player_image);
    sprite_turn_to(player, 0.1, 0.0);
    sprite_turn(player, 0);
    respawn = sprite_create(20, -10, 7, 4, player_respawn);
    safe_block[0] = sprite_create(screen_width() - 8, 10, 7, 2, block_image);
    sprite_turn_to(safe_block[0], 0.1, 0.0);
    sprite_turn(safe_block[0], 0);
    treasure[0] = sprite_create(screen_width() - 5, screen_height()-3, 2, 2, treasure_image);
    sprite_turn_to(treasure[0], 0.1, 0.0);
    sprite_turn(treasure[0], 0);
}

void restart(void) {
    clear_screen();
    maxrow1 = 0;
    start_time = get_current_time();
    game_over = false;
    num_blocks = 10;
    update_screen = true; /* Set to false to prevent screen update. */
    paused = false;
    lives = 10;
    score = 0;
    safe_block_count = 1;
    death_block_count = 0;
    max_safe_blocks = 0;
    num_zombies = 180;
    runtime = 0;

    setup();
}

void end_game() {
    int screenheight = screen_height();
    int screenwidth = screen_width();
    game_over = true;
    static char *msg_text = "Game over!";
    int msg_width = strlen(msg_text) / 2;
    int msg_height = 2;
    int msg_x = (screen_width() - msg_width) / 2;
    int msg_y = (screen_height() - msg_height) / 2;
    sprite_id msg = sprite_create(msg_x, msg_y, msg_width, msg_height, msg_text);
    clear_screen();
    sprite_draw(msg);
    draw_string(screenwidth/2 + 30, screenheight/2 - 14, "Press Q to quit the game.");
    draw_string(screenwidth/2 - 55, screenheight/2 - 14, "Press r to restart the game.");
    draw_string(49, 20, "Time: ");
    draw_formatted(55,20,"%02d:%02d", elapsed_time/60, elapsed_time%60);
    draw_string(62, 20, "Score: ");
    draw_double(69, 20, score);
    show_screen();
    timer_pause(1000);
    bool end_game = false;
    while (end_game == false)
    {
        if (get_char() == 'q') {
            end_game = true;
        }
        if (get_char() == 'r') {
            restart();
            end_game = true;
        }
}
}

void player_lives(void) {
    lives -= 1;
    if (lives == 0) {
        end_game();
    }
    }


void draw_sprites(sprite_id sids[], int n)
{
    for (int i = 0; i < n; i++) {
        sprite_draw(sids[i]);
    }
}

void draw_platform_auto_move(sprite_id sids[], int n, int keyCode, char * sprite_image) {
    for (int i = 0; i < n; i++) {
        auto_move_platform(sids[i], keyCode, sprite_image);
    }
}

void draw_all(void) {
    clear_screen();
    draw_hud();
    sprite_draw(safe_block[0]);
    draw_sprites(safe_block, safe_block_count);
    draw_sprites(death_block, death_block_count);
    sprite_draw(player);
    sprite_draw(treasure[0]);
    sprite_draw(respawn);
}

bool respawning = false;
int respawningtimer = 0;
void player_respawn_logic(void) {
    //int x = round(sprite_x(safe_block[0]));
    if (respawning == true) {
        gravity = false;

        //respawn = sprite_create(round(sprite_x(safe_block[0])), round(sprite_y(safe_block[0])) - 5, 7, 4, player_respawn);
        player = sprite_create( 30, -10, 3, 4, player_image);
        //sprite_draw(respawn);
    }
}


void game_keys(int keyCode) {
    if (keyCode == 'q') {
        end_game();
    }
    if (keyCode == 't') {
        if (run_treasure == 0) {
            sprite_turn_to(treasure[0], 0.0, 0.0);
            run_treasure += 1;
        }
        else {
            sprite_turn_to(treasure[0], 0.1, 0.0);
            run_treasure = 0;
        }
    }
    if (keyCode == 'r') {
        restart();
    }
}

void game_collision_detection(int px, int py) {
    if (sprites_collide_any(player, safe_block, safe_block_count) {
        player = sprite_create(px, py - 1, 3, 4, player_current_img);
        score+=1;        
    }   
    if (sprites_collide_any(player, death_block, death_block_count) || py >= screen_height() - 3 || px <= 1 || px >= screen_width() - 4)  {
        player_lives();
        respawning = true;
        player_respawn_logic();
    }
    if (sprites_collide_any(player, treasure, 1)) {
        lives += 3;
        respawning = true;
        player_respawn_logic();
    }
}

void physics() {
    srand(time(NULL));
    int respawn_location = rand() % maxrow1;
     if (respawning == true) {
        respawningtimer += 1;
        respawn = sprite_create(round(sprite_x(safe_block[respawn_location])), round(sprite_y(safe_block[respawn_location])) - 5, 7, 4, player_respawn);
        if (respawningtimer == 100) {
            respawning = false;
            respawningtimer = 0;
            gravity = true;
            respawn = sprite_create(20, -10, 7, 4, player_respawn);
            player = sprite_create(round(sprite_x(safe_block[respawn_location])), round(sprite_y(safe_block[respawn_location])) - 5, 3, 4, player_image);
        }
    }
    else {
        if (gravity == true) {
            sprite_move(player, 0, +1);
        }
        else if (gravity == false) {
            if (playerjump == true) {
                gravofftime += 1;
                if (gravofftime == 90) {
                    gravity = true;
                    playerjump = false;
                    gravofftime = 0;
                }
            }
        }
    }
}

void process(void) {
    int keyCode = read_char();
    int px = round(sprite_x(player));
    int py = round(sprite_y(player));
    game_keys(keyCode);
    game_collision_detection(px, py);
    physics();
    auto_move(player, keyCode);
    move_player(keyCode);
    auto_move(treasure[0], keyCode);
    draw_platform_auto_move(safe_block, safe_block_count, keyCode, block_image);
    draw_platform_auto_move(death_block, death_block_count, keyCode, block_death_image);
    draw_all();
}

void cleanup(void)
{
    // STATEMENTS
}

int main(void) {
    start_time = get_current_time();
    setup_screen();
    setup();
    show_screen();
    while ( !game_over ) {
        timer(); 
		process();
		if ( update_screen ) {
			show_screen();
		}
		timer_pause(DELAY);
	}
    cleanup();
    return 0;
}