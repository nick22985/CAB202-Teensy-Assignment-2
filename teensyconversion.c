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
#include <usb_serial.h>
#include "cab202_adc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define DELAY (10)  
#define MAX_BLOCK (200)
#define FREQ     (1000000.0)
#define PRESCALE (8.0)
#define THRESHOLD 512
#define sprite_move_to(sprite,_x,_y) (sprite.x = (_x), sprite.y = (_y))

void usb_serial_send(char * message);

int score = 0;
int lives = 10;
double start_time = 0;
bool game_pause = false;
bool treasure_paused = false;
bool pause = false;
int game_over = 1;
int update_screen = 0; 
bool treasure_moving_right = true;
volatile uint32_t overflow_count = 0;
volatile uint32_t over_flow_count = 0; 
int safe_block_count = 0;
int plat_chance = 0;
int block_count = 0;
int run_time = 0;
int food_count = 0;
int food_in_inv = 5;

Sprite zombie[5];
Sprite food[5];
Sprite platforms[60];
Sprite treasure;
Sprite player;
Sprite respawn;


int screen_width = LCD_X;
int screen_height = LCD_Y;
int row_count = (LCD_Y / 10);
char buffer[20];
int rows[(LCD_Y / (5+5))];
int max_blocks;
bool is_restart = false;

bool movement = false;
bool no_control_this_run = false;
int nocontroltimer = 0;
bool player_left = false;
bool player_right = false;
bool food_drop = false;
bool disable_controls = false;
bool player_has_previously_hit_block = false;
int detection_count = 0;
int hit_block_prev = 0;


int zombie_counter = 0;



// -------------------------------------------------
// Helper functions.
// -------------------------------------------------
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

void draw_double(uint8_t x, uint8_t y, double value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%f", value);
	draw_string(x, y, buffer, colour);
}

void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%d", value);
	draw_string(x, y, buffer, colour);
}

// AMS Sync Uart Data
void uart_init(int baud) {
	UBRR1 = (F_CPU / 4 / baud - 1) / 2;
	UCSR1A = (1 << U2X1);
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

void uart_put_char(unsigned char data){
	    while (! ( UCSR1A & (1<<UDRE1)));
	    UDR1 = data;
}

unsigned char uart_receive(void){
    while ( !(UCSR1A & (1<<RXC1)) );
    return UDR1;
}

// -------------------------------------------------

unsigned char player_respawn[] = {
    0b11000110,
    0b01101100,
    0b00111000,
    0b01101100,
    0b11000110,
    };

unsigned char player_image[] = {
    0b00011000,
    0b00111100,
    0b01011010,
    0b00101000,
    0b01000100,
    };

unsigned char safe_platform_bitmap[] = {
    0b11111111, 0b11110000,   
    0b11111111, 0b11110000
};

unsigned char unsafe_death_bitmap[] = {
    0b10101010, 0b10000000,   
    0b01010101, 0b01000000
};

unsigned char treasure_image[] = {
    0b00000000,
    0b00100000,
    0b01010000,
    0b00100000,
    0b00000000,
    };
unsigned char treasure_image_2[] = {
    0b00000000,
    0b01010000,
    0b00100000,
    0b01010000,
    0b00000000
    };

unsigned char food_bitmap[] = {
    0b01100000,
    0b00011000,
    0b00111100,
    0b00111100,
    0b00000000
    };

unsigned char zombie_bitmap[] = {
    0b00011000,
    0b00111100,
    0b00111100,
    0b00011000,
    0b00011000
    };

void timer_setup(){
    //Timer setup
    TCCR3A = 0;
    TCCR3B = 3;
    TCCR0A = 0;
	TCCR0B = 4; 
	//	(b) Enable timer overflow for Timer 3.
    TIMSK0 = 1; 
	TIMSK3 = 1;
    // (c) Turn on interrupts.
    sei();
}

// AMS Topic 9 3
// AMS Week 9 3
volatile uint32_t switch_counter[7]; 
volatile uint8_t pressed[7];
ISR(TIMER0_OVF_vect) {
    for (int i = 0; i < 7; i++) {
        switch_counter[i] = switch_counter[i] << 1;
        uint8_t mask = 0b00000111;
        switch_counter[i] &= mask;
        switch_counter[0] |= BIT_IS_SET(PINB, 0); // center
        switch_counter[1] |= BIT_IS_SET(PINB, 1); //left
        switch_counter[2] |= BIT_IS_SET(PIND, 0); // right
        switch_counter[3] |= BIT_IS_SET(PIND, 1); //up
        switch_counter[4] |= BIT_IS_SET(PINB, 7); //down
        switch_counter[5] |= BIT_IS_SET(PINF, 6); //SW2
        switch_counter[6] |= BIT_IS_SET(PINF, 5); //SW3
        if (switch_counter[i] ==  mask)
        {
            pressed[i] = 1;
        }
        else if (switch_counter[i] == 0)
        {
            pressed[i] = 0;
        }
    }
}

ISR(TIMER3_OVF_vect) {
	over_flow_count ++;
}

double get_elapsed_time() {
    double elapsed_time = ( over_flow_count * 65536.0 + TCNT3 ) * PRESCALE / FREQ;
    return elapsed_time;
}

void setup_controler(void) {
    int start_game = 0;
    CLEAR_BIT(DDRB,1); //left
    CLEAR_BIT(DDRD,0); //right
    CLEAR_BIT(DDRD,1); //up
    CLEAR_BIT(DDRB,7); //down
    CLEAR_BIT(DDRB,0); //Center
    CLEAR_BIT(DDRF,6); //SW2
    CLEAR_BIT(DDRF,5); //SW3
    for (int i = 0; i < 7; i++) {
        switch_counter[i] = 0;
    }
    // Timers
    timer_setup();
    //sets cpu to 8mhx
    set_clock_speed(CPU_8MHz);
    //initialising the lcd screen
    lcd_init(LCD_DEFAULT_CONTRAST);
    //clears scereen
    clear_screen();
    //write the string on the lcd
    while (start_game == 0) {
        draw_string(5,10,"Nicholas", FG_COLOUR);
        draw_string(5,20,"Dempsey", FG_COLOUR);
        draw_string(5,30,"n10224378", FG_COLOUR);
        show_screen();
        if (BIT_IS_SET(PINF, 6)) {
            start_game = 1;
            over_flow_count = 0;
            clear_screen();
        }
    }
}

void draw_hud() {
    clear_screen();
    char livesbuff[20];
    char scorebuff[20];
    char timebuff[20];
    char zombbuff[10];
    char foodbuff[10];
    int time_in_int_form = (int) get_elapsed_time();
    sprintf(livesbuff, "Lives: %02d", lives);
    sprintf(scorebuff, "Score: %02d", score);
    draw_string(0, 0, livesbuff, FG_COLOUR);
    draw_string(0, 10, scorebuff, FG_COLOUR);
    draw_string(0, 20, "Time:", FG_COLOUR);
    sprintf(timebuff, "%02d:%02d", time_in_int_form/60, time_in_int_form%60);
    draw_string(25, 20, timebuff, FG_COLOUR);
    sprintf(scorebuff, "Zombie: %02d", zombie_counter);
    sprintf(scorebuff, "Food: %02d", food_count);
    draw_string(0, 30, zombbuff, FG_COLOUR);
    draw_string(0, 40, foodbuff, FG_COLOUR);
    show_screen();
}

void place_food(){
    if (food_in_inv > 0){
        food_in_inv -= 1;
        sprite_init(&food[food_count], (player.x), (player.y - 5), 4, 4,food_bitmap);
        food[food_count].is_visible = true;
        food_count += 1;
    }
}

void keyboard(int key_code) {
    if ( BIT_IS_SET(PIND, 1) ) key_code = 'w';
	if ( BIT_IS_SET(PINB, 7) ) key_code = 's';
	if ( BIT_IS_SET(PINB, 1) ) key_code = 'a';
	if ( BIT_IS_SET(PIND, 0) ) key_code = 'd';
    
    if ( key_code > 0 ) {
		usb_serial_putchar(key_code);
	}
}

void controls(void) {
    int key_code = -1;
    keyboard(key_code);
    if (disable_controls == false) {
		if ( pressed[0] == 1){
            //center
            pause = true;
            while(pause == true) {
                clear_screen();

                draw_hud();
                show_screen();
                _delay_ms(500);
                if (BIT_IS_SET(PINB, 0)) {
                    pause = false;
                    _delay_ms(500);
                }
            }
		} 
        if ( pressed[1] == 1){
            //left
            player.x -= 1;
            movement = true;
            player_left = true;
            nocontroltimer = 0;
        }
        if ( pressed[2] == 1){
            //right
            player.x += 1;
            movement = true;
            player_right = true;
            nocontroltimer = 0;
        }
        if ( pressed[3] == 1){
            //up
            player.dy -= 2.5;
            movement = true;
            nocontroltimer = 0;
        }
        if ( pressed[4] == 1){
            //down
            movement = true;
            nocontroltimer = 0;            
        }
        if ( pressed[5] == 1){
            //SW2
        }
        if ( pressed[6] == 1){
            // SW3
            for (int i = 0; i < 30000; i++) {};
            treasure_paused = !treasure_paused;
        }
        else {
            nocontroltimer += 1;
        }
    }
}

void block_direction(int counter) {

        if (run_time == 0) {
            sprite_turn_to(&platforms[0], 0.1, 0);
            sprite_turn(&platforms[0], 0);
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

bool detection = true;
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
    if (l1 > r2) {
        detection = false;
        return false;
    }
    if (l2 > r1) {
        detection = false;
        return false;
    }
    if (t1 > b2) {
        detection = false;
        return false;
        }
    if (t2 > b1) {
        detection = false;
        return false; 
        }
    detection = true;
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

void setup_platforms(){
    int gap_blocks = 2;
    double block_placment_chance;
    double block_x = 70;
    double y = 12;
    int counter = 1;
    for (int i = 0; i < row_count; i++){
        block_x = 0;
        block_count = 0;
        while(block_x < screen_width - 12){
            block_placment_chance = rand() % 100;
            if(block_placment_chance < 60){
                sprite_init(&platforms[counter], block_x + gap_blocks, y, 10, 2, safe_platform_bitmap );
                block_direction(counter);
            }
            if(block_placment_chance > 60 && block_placment_chance < 95){
                sprite_init(&platforms[counter], block_x + gap_blocks, y, 10, 2, unsafe_death_bitmap );
                block_direction(counter);
            }
            else{
                block_x = block_x + 10;
            }
            counter++;
            block_count += 1;
            block_x = block_x + 20;
        }
        run_time += 1;
        rows[i] = block_count;
        max_blocks += block_count;
        y = y + 12;
    }
    block_x = 0;
}

void draw_platform(){
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
        treasure.x = treasure.x;
    }
    else if (treasure_paused == false){
        if (treasure_moving_right == true){
            treasure.x += 1;
            if(treasure.x == LCD_X - 2){
                treasure_moving_right = false;
            }
        }
        if (treasure_moving_right == false){
            treasure.x -= 0.5;
            if(treasure.x == 1){
            treasure_moving_right = true;
            }
        }
    }
}

void block_move(){
    for (int i = 0; i < max_blocks; i++) {
        // int left_adc = adc_read(0);
	    // int right_adc = adc_read(1);
        // sprite_move_to(platforms[i],(double) left_adc * (LCD_Y - platforms[i].height),(double) right_adc * (LCD_Y - platforms[i].height));
        sprite_step(&platforms[i]);
        if(platforms[i].x >= LCD_X){
            platforms[i].x = 1;
        }
        if (platforms[i].x < 0) {
            platforms[i].x = LCD_X;
        }
    }
}
void teleport_zombie() {
     for (int i = 0; i < 4; i++) {
        sprite_step(&zombie[i]);
        if(zombie[i].x >= LCD_X){
            zombie[i].x = 1;
        }
        if (zombie[i].x < 0) {
            zombie[i].x = LCD_X;
        }   
     }
}

void draw_zombies(void) {
    int time_in_int_form = get_elapsed_time();
    if (time_in_int_form > 3){
        for (int i = 0; i < 5; i++)  {
            sprite_draw(&zombie[i]);
            zombie_counter += 1;
        }
    }
}

void spawn_zombies() {
    int x = 0;
    int y = - 10;
        for (int i = 0; i < 5; i++)  {
            sprite_init(&zombie[i], x += 10, y, 5, 4, zombie_bitmap);
        }
    }

void create_all_sprites() {
    sprite_init(&player, screen_width - 10, 5, 8, 5, player_image);
    sprite_init(&treasure, 2, (LCD_Y-5), 3, 3, treasure_image);
    sprite_init(&platforms[0], (screen_width- 12), 12, 10, 2, safe_platform_bitmap);
    for (int i = 0; i < 4; i++) {
        sprite_init(&food[i], 0, 0, 4, 4, food_bitmap);
        food[i].is_visible = false;
    }
    setup_platforms();
    spawn_zombies();
}

void draw_food(){
    for (int i = 0; i < 5; i++){
        sprite_draw(&food[i]);
    }
}

void draw_all() {
    sprite_draw(&player);
    draw_food();
    sprite_draw(&treasure);
    draw_platform();
    draw_zombies();
}

void move_all() {
    treasure_move();
    block_move();
    teleport_zombie();
}

void restart (void) {
    if (is_restart == true) {
        score = 0;
        lives = 10;
        start_time = 0;
        game_pause = false;
        treasure_paused = false;
        pause = false;
        game_over = 1;
        update_screen = 0; 
        treasure_moving_right = true;
        overflow_count = 0;
        over_flow_count = 0;
        safe_block_count = 0;
        plat_chance = 0;
        block_count = 0;
        run_time = 0;
        food_count = 4;
        player_has_previously_hit_block = false;
        detection_count = 0;
        hit_block_prev = 0;
        setup_controler();
        create_all_sprites();
        srand(get_elapsed_time() * 100000);
        show_screen();
    }
}

void end_game() {
    char livesbuff[20];
    char scorebuff[20];
    char timebuff[20];
    int time_in_int_form = (int) get_elapsed_time();
    clear_screen();
    draw_string(0, 1, "Game Over!", FG_COLOUR);
    sprintf(livesbuff, "Lives: %02d", lives);
    sprintf(scorebuff, "Score: %02d", score);
    draw_string(0, 11, livesbuff, FG_COLOUR);
    draw_string(0, 21, scorebuff, FG_COLOUR);
    draw_string(0, 31, "Time:", FG_COLOUR);
    sprintf(timebuff, "%02d:%02d", time_in_int_form/60, time_in_int_form%60);
    draw_string(25, 31, timebuff, FG_COLOUR);
    show_screen();
    bool end_game_screen = true;
    while (end_game_screen == true)
    {
        if (BIT_IS_SET(PINF,5)){
            // SW3
        game_over = true;
        }
        if (BIT_IS_SET(PINF,6)){
            // SW2
            is_restart = true;
            restart();
            end_game_screen = false;
        }
    }
}

void player_lives(void) {
    lives -= 1;
    if (lives <= 0) {
        end_game();
    }
}


void player_respawn_logic() {
    sprite_init(&player, platforms[0].x, platforms[0].y - 5, 8, 5, player_image);
}

void zombie_logic (void) {
    for (int i = 0; i < 4; i++) {
    if (zombie[i].x >= LCD_X - 2) {
        zombie[i].x = - 5;
    }
    }

}

void score_phys() {
        if (player_has_previously_hit_block == true) {
            detection = true;
            player_has_previously_hit_block = false;
            detection_count += 1; 
            hit_block_prev += 1;
        }
        if (detection_count > 50) {
            if (hit_block_prev > 50) {
                score += 1;
                detection_count = 0;
                hit_block_prev = 0;
            }
        }
    }

void gravity(Sprite platforms_collided) {
    if (nocontroltimer > 5) {
        movement = false;
        nocontroltimer = 0;
    }
    if (movement == false) {
        if (player_has_previously_hit_block == false) {
            player.dx = 0;
        }
        if (platforms_collided.dx < 0) {
            //--
        }
           if (platforms_collided.dx > 0) {
            // ++
        }
        player.dx = platforms_collided.dx;
    }
    if (movement == true) {
        
        if (player_left == true){
            player.dx -= 0.1;
            player_left = false;
        }
        if (player_right == true){
            player.dx += 0.1;
            player_right = false;
        }
    }
    player.y = player.y + player.dy;
    player.x = player.x + player.dx;
    sprite_step(&player);
}

void game_physics(Sprite platforms_collided) {
    score_phys();
    gravity(platforms_collided);
   // zombie_logic();
}

void collision_detection() {
    Sprite platforms_collided = sprites_collide_any(player, platforms, max_blocks);
    // Sprite zombie_with_player_collide = sprites_collide_any(player, zombie, 4);
    if (platforms_collided.bitmap == safe_platform_bitmap) {
        player.y -= 2;
        player_has_previously_hit_block = true;
        disable_controls = false;
    }
    if (platforms_collided.bitmap != safe_platform_bitmap) {
        disable_controls = true;
    }
    if (platforms_collided.bitmap == unsafe_death_bitmap || player.y >= LCD_Y  || player.x >= LCD_X - 2 || player.x < 0) {
        player_lives();
        player_respawn_logic();
    }
    // if (zombie_with_player_collide.bitmap == zombie_bitmap) {
    //     player_lives();
    //     player_respawn_logic();
    // }
    
    for ( int i = 0; i < 4; i++) {
        Sprite zombie_collided = sprites_collide_any(zombie[i], platforms, max_blocks);
        if (zombie_collided.bitmap == safe_platform_bitmap) {
            zombie[i].y -= 0.2;
            if (detection == true) {
                zombie[i].dx = zombie_collided.dx;
            }  
        }
    }

    if (sprites_collide(player, treasure) == true ) {
        player_respawn_logic();
        lives += 2;
        treasure_paused = true;
        sprite_init(&treasure, -5, -5, 3, 3, treasure_image);
        treasure.is_visible = false;
    }

    game_physics(platforms_collided);
    player.dy = 1;
    for (int i = 0; i <= 4; i++) {
        zombie[i].dy = 0.1;
        sprite_step(&zombie[i]);
    }
    
     for (int i = 0; i <= food_count; i++) {
        Sprite food_collided = sprites_collide_any(food[food_count], platforms, max_blocks);
         if (food_collided.bitmap == safe_platform_bitmap) {
             food[food_count].y -= 2;
         }
         food[food_count].y += 1;
     }
}

void process() 
{
    controls();
    clear_screen(); 
    move_all();
    draw_all();
    collision_detection();
    show_screen();
}
// void adc_setup() {
//     adc_init();

// }

// #define sprite_move_to(sprite,_x,_y) (sprite.x = (_x), sprite.y = (_y))

void setupusbserial() {
    usb_init();
	while (!usb_configured()) {
			
	}

}

void setup(void) {
    setupusbserial();
    setup_controler();
    create_all_sprites();
    //draw_hud();
    srand(get_elapsed_time());
    show_screen();
}


int main(void) {

    setup();
    usb_serial_send("Game Start ");
    usb_serial_putchar(player.x);
    usb_serial_send(", ");
    usb_serial_putchar(player.y);
    while ( !game_over == false ) {
		process();
		if ( update_screen == true ) {
			show_screen();
		}
	}

    return 0;
}


void usb_serial_send(char * message) {
	// Cast to avoid "error: pointer targets in passing argument 1 
	//	of 'usb_serial_write' differ in signedness"
	usb_serial_write((uint8_t *) message, strlen(message));
}
