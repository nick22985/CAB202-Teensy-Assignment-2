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
#define STARTING_LIVES 10

//timer stuff
#define FREQ     (1000000.0)
#define PRESCALE (8.0)
volatile uint32_t overflow_count = 0; 
char buffer[20];

//control bools
bool left = false;
bool right = false;
bool up = false;
bool down = false;
bool stick_in = false;
bool SW2 = false;
bool SW3 = false;
//global vars

bool on_start_screen = true;
bool paused = false;




int player_lives = STARTING_LIVES;
int player_score = 0;

//used to monitor x coord of the treasure to determine when it should turn around. obviously no Y coord, as it's on a flat trajectory.
int treasure_x;

//toggled by 'SW3', keeps the treasure stationary and unanimated.
bool treasure_paused = false;
bool treasure_moving_right = true;
//Is the player falling?
bool gravity = false;
// Does the player have something to propel himself against? 
bool grounded = true; // should start false, is set to true when collision with block
bool game_over = false;

bool sprite_step( sprite_id sprite ) {
	int x0 = round( sprite->x );
	int y0 = round( sprite->y );
	sprite->x += sprite->dx;
	sprite->y += sprite->dy;
	int x1 = round( sprite->x );
	int y1 = round( sprite->y );
	return ( x1 != x0 ) || ( y1 != y0 );
}

void sprite_turn_to( sprite_id sprite, double dx, double dy ) {
	sprite->dx = dx;
	sprite->dy = dy;
}

void sprite_turn( sprite_id sprite, double degrees ) {
	double radians = degrees * M_PI / 180;
	double s = sin( radians );
	double c = cos( radians );
	double dx = c * sprite->dx + s * sprite->dy;
	double dy = -s * sprite->dx + c * sprite->dy;
	sprite->dx = dx;
	sprite->dy = dy;
}


//bitmaps

unsigned char player_bitmap[] = {
    0b01000000,
    0b11100000,
    0b01000000,
    0b10100000
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
Sprite static_block[10]; //what are these
Sprite platforms[50];
 

int screen_width = LCD_X;
int screen_height = LCD_Y;
int row_count = (LCD_Y / (5+5));
int row_count;
int block_count;
int max_blocks = 0;

int row_static; //ask oscar
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
double get_elapsed_time() {
    return (overflow_count * 65536.0 + TCNT3 ) * 64.0 / 8000000.0;
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
        if (grounded == true){
            player.y = player.y +0.5;
        }
        
        //down
        }
    if (BIT_IS_SET(PINB,0)){
        stick_in = true;
    }
        else{
        stick_in = false;
        }
    if (BIT_IS_SET(PINF,6)){
        //SW2
        }
    if (BIT_IS_SET(PINF,5)){
        treasure_paused = !treasure_paused;
        }
    else if (grounded == false){
        player.y = player.y +0.1;
    }
}





void refresh_screen(){
    clear_screen();
	show_screen();
}


//collision management
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


void collision_detection(){
    Sprite platforms_collided = sprites_collide_any(player, platforms, max_blocks);
    if (platforms_collided.bitmap == safe_platform_bitmap){
        player_score = player_score + 1; //should be score_point();
        player.y -= 2;  
        gravity = false;
        grounded = true;
    }
    else if (platforms_collided.bitmap == unsafe_platform_bitmap){
        //player_death();  //MAKE PLAYER DEATH
        gravity = false;
        grounded = false;
    }
    else if (platforms_collided.bitmap == treasure_bitmap){
        //respawn_player(); //MAKE RESPAWN PLAYeR
        player_lives = player_lives + 2;
        grounded = false;
    }
}

int run_time;

void block_direction(int counter) {
         if (run_time == 0) {
            sprite_turn_to(&platforms[counter], 0.1, 0);
            sprite_turn(&platforms[counter], 0);
        }
        if (run_time == 1) {
            sprite_turn_to(&platforms[counter], -0.1, 0);
            sprite_turn(&platforms[counter], 0);
        }
        if (run_time == 2) {
            sprite_turn_to(&platforms[counter], 0.1, 0);
            sprite_turn(&platforms[counter], 0);
            run_time = 0;
        }
}

void place_platforms(){
    int gap_between_blocks = 2;
    double placement_chance;
    double bx = 74;
    double by = 12;
    int counter = 1;
    for (int i = 0; i < row_count; i++){
        bx = 0;
        block_count = 0;
        while(bx < screen_width - 12){
            placement_chance = rand() % 100;
            if(placement_chance < 60){
                sprite_init(&platforms[counter], bx + gap_between_blocks, by, 10, 2, safe_platform_bitmap );
                block_direction(counter);
            }
            if(placement_chance > 60 && placement_chance < 95){
                sprite_init(&platforms[counter], bx + gap_between_blocks, by, 10, 2, unsafe_platform_bitmap );
                block_direction(counter);
            }
            else{
                bx = bx + 10;
            }
            counter++;
            block_count += 1;
            bx = bx + 20;
        }
        run_time += 1;
        rows[i] = block_count;
        max_blocks += block_count;
        by = by + 10;
    }
    bx = 0;
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

void treasure_move(){
    if (treasure_paused == true){

    }
    else if (treasure_paused == false){
        if (treasure_moving_right == true){
            treasure.x = treasure.x +0.1;
            if(treasure.x >= LCD_X-2){
                treasure_moving_right = false;
            }
        }
        if (treasure_moving_right == false){
            treasure.x = treasure.x -0.1;
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
    show_screen();

}


void setup(void) {
	set_clock_speed(CPU_8MHz);
	lcd_init(127);
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
void process(){
    clear_screen();    
    draw_stuff();
    controls();
    treasure_move();
    collision_detection();
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