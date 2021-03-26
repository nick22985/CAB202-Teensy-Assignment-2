#include <avr/io.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <macros.h>
#include <graphics.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <lcd.h>
#include "lcd_model.h"
#include "sprite.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
//#include <usb_serial.h>
#define STARTING_LIVES 1

//timer stuff
#define FREQ     (1000000.0)
#define PRESCALE (8.0)
volatile uint32_t overflow_count = 0; 
//char buffer[20];

//global vars

bool on_start_screen = true;
bool paused = false;

//char livesbuff[20];
//char scorebuff[80];
//char timebuff[80];


int player_lives = STARTING_LIVES;
int player_score = 0;
int food_in_inventory = 5;
int food_counter = 0;

//used to monitor x coord of the treasure to determine when it should turn around. obviously no Y coord, as it's on a flat trajectory.
int treasure_x;

//toggled by 'SW3', keeps the treasure stationary and unanimated.
bool treasure_paused = false;
bool treasure_moving_right = true;
//Is the player falling?
bool gravity = false; // should start false
// Does the player have something to propel himself against? 
bool grounded = false; // should start false, is set to true when collision with block
bool game_over = false;


//bitmaps

unsigned char player_bitmap[] = {
    0b01000000,
    0b11100000,
    0b01000000,
    0b10100000
};
unsigned char food_bitmap[] = {
    0b11100000,
    0b11100000
};

unsigned char safe_platform_bitmap[] = {
    0b11111111, 0b11110000,   
    0b11111111, 0b11110000
};

unsigned char unsafe_platform_bitmap[] = {
    0b10101010, 0b10000000,   
    0b01010101, 0b01000000
};

unsigned char treasure_bitmap[] = {
    0b11000000,
    0b11000000
};
//sprite IDs

Sprite player;
Sprite treasure;
Sprite start_platform;
Sprite platforms[65];
Sprite food[15];

 

int screen_width = LCD_X;
int screen_height = LCD_Y;
int row_count = (LCD_Y / (5+5));
int row_count;
int block_count;
int max_blocks =1;

int row_static;
int rows[(LCD_Y / (5+5))];
int safe_platform_count = 1;
int unsafe_platform_count = 0;

void timer_setup(){
    //Timer setup
    TCNT0 = 0;
    TCCR3A = 0;
    TCCR3B = 3;
	//	(b) Enable timer overflow for Timer 1.
    TIMSK3 = 1;
	//	(c) Turn on interrupts.
	sei();
}
ISR(TIMER3_OVF_vect) { 
	overflow_count ++;
}
//from AMS
// void draw_double(uint8_t x, uint8_t y, double value, colour_t colour) {
//     snprintf(buffer, sizeof(buffer), "%f", value);
//     draw_string(x, y, buffer, colour);
// }

// void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
//     snprintf(buffer, sizeof(buffer), "%d", value);
//     draw_string(x, y, buffer, colour);
// }



double get_elapsed_time() {
    return (overflow_count * 65536.0 + TCNT3 ) * 64.0 / 8000000.0;
}

void place_food(){
    if(grounded == true){
        if (food_in_inventory > 0){
            food_in_inventory = food_in_inventory-1;
            sprite_init(&food[food_counter], (player.x), (player.y+2), 3, 2,food_bitmap);
            food_counter = food_counter+1;
        }

    }
}

void controls(){
    //DEBOUNCE THESE
    if (BIT_IS_SET(PINB,1)){
        if (grounded == true){
        player.x = player.x -0.5;
  
        }        
        //left
        }
    if (BIT_IS_SET(PIND,0)){
        if (grounded == true){
            player.x = player.x +0.5;
        }        
        
        //right
        }
    if (BIT_IS_SET(PIND,1)){
        if (grounded == true){
            player.y = player.y -0.5;
        }        
        
        //up
        }
    if (BIT_IS_SET(PINB,7)){ 
        place_food();       
        //down
        }
    if (BIT_IS_SET(PINB,0)){
        paused = true;
    }
    if (BIT_IS_SET(PINF,6)){
        //SW2
        }
    if (BIT_IS_SET(PINF,5)){
        treasure_paused = !treasure_paused;
        }

}

//collision management // from AMS/my assignment 1, ported to teensy compatible language
bool sprites_collide(Sprite s1, Sprite s2)
{
    if ((!s1.is_visible) || (!s2.is_visible)) {
        return false;
    }

    int l1 = s1.x;
    int l2 = s2.x;
    int t1 = s1.y;
    int t2 = s2.y;
    int r1 = l1 + s1.width;
    int r2 = l2 + s2.width - 1;
    int b1 = t1 + s1.height;
    int b2 = t2 + s2.height -1;

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

Sprite sprites_collide_any(Sprite s, Sprite sprites[], int n)
{
    Sprite result;

    for (int i = 0; i < n; i++)
    {
        if (sprites_collide(s, sprites[i]))
        {
            result = sprites[i];
        }
    }

    return result;
}
void respawn_player(){
    player.y = 0;
    player.x = (screen_width- 22);
} 
void restart_game(){
    clear_screen();
    player_lives = STARTING_LIVES;
    player_score = 0;
    respawn_player();
    food_in_inventory = 5;
    food_counter = 0;
    treasure_paused = false;
    show_screen();

}
void game_over_screen(){
    game_over = true;
    bool finished = false;
    while(game_over == true){
        clear_screen();
        draw_string(5, 2, "GAME OVER", FG_COLOUR); 
        draw_string(5, 11, "SW2 to quit", FG_COLOUR); 
        draw_string(5, 19, "SW3 to restart", FG_COLOUR); 
        //score
        //game time
        show_screen();
        if (BIT_IS_SET(PINF,6)){
            finished = true;
            while (finished == true){
                clear_screen();
                draw_string(5, 2, "n8846855", FG_COLOUR);
                draw_string(5, 10, "Thanks for", FG_COLOUR);
                draw_string(5, 17, "playing ", FG_COLOUR);
                draw_string(5, 25, ". . ", FG_COLOUR);
                draw_string(5, 32, " U ", FG_COLOUR);
                show_screen();
            }

        }
        if (BIT_IS_SET(PINF,5)){
        game_over = false;
        restart_game();
        }
    }
}



// from my assignment 1, ported to teensy compatible language
void player_death(){
    player_lives = player_lives -1;
    if (player_lives > 0){
        respawn_player();
    }
    else{
        game_over_screen();
    }
}

// from my assignment 1, ported to teensy compatible language
void collision_detection(){
    int py = player.y; //can this be removed?
    Sprite temp_platform = sprites_collide_any(player, platforms, max_blocks);
    if (temp_platform.bitmap == safe_platform_bitmap){
        player_score = player_score + 1; //should be score_point();
        gravity = false;
        grounded = true;
    }
    else if (temp_platform.bitmap == unsafe_platform_bitmap){
        player_death(); 
        gravity = false;
        grounded = false;

    }
    else if (sprites_collide(player, treasure) == true){
        respawn_player();
        player_lives = player_lives + 2;
        grounded = false;
        treasure.x = -10;
        treasure.y = -10;
        treasure_paused = true;

    }
    else{
        gravity = true;
        if (gravity == true) {
            player.y = player.y +0.2;
            grounded = false;
            if (py >= LCD_Y){
            player_death(); 
            }       
        }
        
    }
}

void place_platforms(){

    int gap_between_blocks = 2;
    double placement_chance;
    double bx = 70;
    double by = 11;

    int counter = 1;
    for (int i = 0; i < row_count; i++){
        bx = 0;
        block_count = 0;
        while(bx < screen_width - 12){
            placement_chance = rand()% 100;
            if(placement_chance < 60){
                sprite_init(&platforms[counter], bx + gap_between_blocks, by, 10, 2,safe_platform_bitmap );
                safe_platform_count = safe_platform_count +1;
            }
            if(placement_chance > 60 && placement_chance < 95){
                sprite_init(&platforms[counter], bx + gap_between_blocks, by, 10, 2,unsafe_platform_bitmap );
                unsafe_platform_count = unsafe_platform_count +1;
            }
            else{
                bx = bx + 10;
            }
            counter++;
            block_count = block_count +1;
            bx = bx + 20;
        }
    rows[i] = block_count;
    by = by + 10 ;
    max_blocks = max_blocks +block_count;
    }
}

void create_sprites(){
    sprite_init(&player, (screen_width- 22), 0, 3, 4, player_bitmap);
    sprite_init(&treasure, 2, (LCD_Y-3), 2, 2, treasure_bitmap);
    sprite_init(&platforms[0], (screen_width- 29), 4, 10, 2, safe_platform_bitmap);
    place_platforms();
}

void draw_platforms(){
    int counter = 0;
    for (int i = 0; i < row_count; i++){
        for (int p = 0; p< rows[i]; p++){
            sprite_draw(&platforms[counter]);
            counter = counter + 1;
        }
    }
}
void draw_food(){
    for (int i = 0; i < (5 - food_in_inventory); i++) 
    sprite_draw(&food[i]);
}
// from my assignment 1, ported to teensy compatible language
void treasure_move(){
    //make treasure invisible and move it offscreen to remove it after collision?
    if (treasure_paused == true){

    }
    else if (treasure_paused == false){
        if (treasure_moving_right == true){
            treasure.x = treasure.x +0.2;
            if(treasure.x >= LCD_X-2){
                treasure_moving_right = false;
            }
        }
        if (treasure_moving_right == false){
            treasure.x = treasure.x -0.2;
            if(treasure.x <= 2){
            treasure_moving_right = true;
                }
            }
        }
    
}
void draw_stuff(){
    clear_screen();
    sprite_draw(&player);
    sprite_draw(&treasure);
    sprite_draw(&start_platform);
    draw_platforms();
    draw_food();
    show_screen();

}


void setup(void) {
	set_clock_speed(CPU_8MHz);
	lcd_init(LCD_DEFAULT_CONTRAST);
    clear_screen();
	show_screen();
    timer_setup();

//allocate memory to buttons

    CLEAR_BIT(DDRB,1); //left
    CLEAR_BIT(DDRD,0); //right
    CLEAR_BIT(DDRD,1); //up
    CLEAR_BIT(DDRB,7); //down
    CLEAR_BIT(DDRB,0); //Stick in
    CLEAR_BIT(DDRF,6); //SW2
    CLEAR_BIT(DDRF,5); //SW3

    place_platforms();
    create_sprites();

}

void start_screen(){
    while (! BIT_IS_SET(PINF,6) && on_start_screen == true){
        clear_screen();
        draw_string(5, 2, "n8846855", FG_COLOUR); 
        draw_string(5, 10, "Dan Kenward", FG_COLOUR); 

        show_screen();

    }
    on_start_screen = false;
    srand(get_elapsed_time());
    create_sprites();
    place_platforms();
    
}
// void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
//     snprintf(buffer, sizeof(buffer), "%d", value);
//     draw_string(x, y, buffer, colour);
// }
void pause_screen(){
    while(paused == true){
            clear_screen(); 
            //draw_int(15,2, player_lives, FG_COLOUR);
            draw_string(5, 2, "lives :", FG_COLOUR);//lives
            draw_string(5, 10, "score :", FG_COLOUR);//score
            draw_string(5, 18, "time :", FG_COLOUR);//time (mm:ss)
            //sprintf (5, 20, "%02d", player_lives, FG_COLOUR);
            //FOOD INVENTORY COUNT
            show_screen();
            if (BIT_IS_SET(PINB,0)){
            paused = false;
        }
    }
}
void process(){
    clear_screen();    
    draw_stuff();
    controls();
    treasure_move();
    collision_detection();
    pause_screen();
    show_screen();
   //start_game();
  

}

int main(void) {
    // usb_init();
    // while (!usb_configured()) {

    // }
	setup();

    start_screen();
    while (!game_over){
        
        process();

    }
    return 0;
	//for ( ;; ) {
        
		//process();
	//}
}