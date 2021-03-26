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

// Timer
#define FREQ     (1000000.0)
#define PRESCALE (8.0)
volatile uint32_t overflow_count = 0; 

// Global variables 
int XC = LCD_X;
int YC = LCD_Y;
bool game_start = false;
int counter = 0;
int lives = 10;

// Create Sprite id
Sprite player;
Sprite blocks;

// Set up player
unsigned char player_img_static[] = {
		0b01001000,
        0b00110000,
        0b11111100, 
        0b00110000,
        0b01001000
    };

unsigned char g_blocks[] = {
        0b11111111, 0b11000000,
        0b11111111, 0b11000000
};

unsigned char b_blocks[] = {
        0b01010101, 0b01000000,
        0b10101010, 0b10000000
};

void timer (void){
	// Timer setup
	TCNT0 = 0;
	TCCR3A = 0;
	TCCR3B = 3;

	//Enable timer overflow for Timer 1.
	TIMSK3 = 1;

	//Turn on interrupts.
	sei();
}

ISR(TIMER3_OVF_vect) { 
	overflow_count ++;
}

double elapsed_time() {
    return (overflow_count * 65536.0 + TCNT3 ) * 64.0 / 8000000.0;
    }

void setup_player (void){
	//Create sprite player
	sprite_init(&player, (XC - 9), 5, 6, 5, player_img_static);

}

void player_movement (void){
	// Left movement 
	if (BIT_IS_SET(PINB, 1)){
		player.x++;
	}
}

void setup_blocks (void){
	// int y_cor = 12;
	// int x_cor = 2;
	// int count = 0;

	// for (int i = 0; i < 3; i++){
	// 	int block_probabilty = rand()%10;
	// 	if(block_probabilty < 75){
	// 		sprite_init(&block, x_cor, y_cor, 10, 2, g_blocks);
	// 	}
	// 	else if(block_probabilty > 75){
	// 		sprite_init(&blocks, x_cor, y_cor 10, 2, b_blocks);
	// 	}
	// 	else{
	// 		y_cor += 10;
	// 	}
	// 	count++


	// Create an array that contains the 2 different type of blocks
	// unsigned char*block_type[4];
	// block_type[1] = g_blocks;
	// block_type[2] = b_blocks;
	// block_type[3] = g_blocks;

	// Variables 
	int y_cor = 12;
	int x_cor = 2;
	//int spacing = rand() % 5;

	// Create rows/ columns of blocks
	for (int i = 0; i < 7; i++){
		y_cor = 12;
		for (int a = 0; a< 3; a++){
			sprite_init(&blocks, x_cor,  y_cor, 10, 76, g_blocks);
			y_cor += 12;
			sprite_draw(&blocks);
		}
		x_cor += 12;
	}

	// sprite_init(&blocks, (XC- 12), 12, 10, 2, g_blocks);
}

void input_setup (void){
	// Configure to allow input into joystick and right button (SW2)
	CLEAR_BIT(DDRF,6); // left button
	CLEAR_BIT(DDRD,1); // Up
	CLEAR_BIT(DDRB,7); // Down
	CLEAR_BIT(DDRB,1); // Left
	CLEAR_BIT(DDRD,0); // Right
	CLEAR_BIT(DDRB,0); // Centre 
}

void setup_game (void){
	// Game setup
	setup_player();
	setup_blocks();

}


void check_button(void){
	// If left button is pressed, game_start = true 
	// true = game start
	if (BIT_IS_SET(PINF, 6)){
    	game_start = true;
    	// srand(timer(NULL));
    }
    else {
    	// Draw main screen
    	draw_string(((XC / 2) - 28), ((YC / 2) - 10), "Kim See-Kee", FG_COLOUR);
    	draw_string(((XC / 2)- 22), YC / 2, "n10269771", FG_COLOUR);
    	show_screen();
    }

}

void gravity (void){
    player.y = player.y + 0.75;
}

void play_game (void){
	// Setup game
	setup_game();
	gravity();
	player_movement();

	// Player
	sprite_draw(&player);

	// Blocks
	
}

void start_game (void){
	// While the game_start is true, game process happens
	while (game_start == true){
		srand(elapsed_time());
		clear_screen();
		play_game();
		show_screen();
	}
}

void process(void){
	clear_screen();

	check_button();

	start_game();
}
void setup(void){
	// Set the CPU speed to 8MHz.
    set_clock_speed(CPU_8MHz);

    // Initialise the LCD display using the default contrast setting
	lcd_init(LCD_DEFAULT_CONTRAST);
	
	input_setup(); // Clearing bits 

	setup_blocks();

	show_screen();
}


int main(void) {
	setup();

	for ( ;; ) {
		process();
		_delay_ms(10);
	}

	return 0; 
	
}