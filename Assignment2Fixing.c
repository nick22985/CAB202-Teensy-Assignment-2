/*
* CAB202 Assignment 2 - Teensy in Space
*
* Author: Tom Stark
* Student Number: 9499776
*/

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "cpu_speed.h"
#include "graphics.h"
#include "small_graphics.h"
#include "sprite.h"
#include "lcd.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Forward declarations of functions
void InitHardware();
void EventLoop();
void SetupStartingValues();
void EventLoop();
void DrawHUD();
void CreateSpaceShip();
void CreateAlien();
void ProcessKey();
void ChooseLevel();
void ProcessLevel();
void MoveAliens();
void ChooseWhereToDropBullet();
void CheckIfThereIsAAlienBulletToDrop();
void MoveVisibleBullets();
void CreateAlienBullets();
void MoveBullet1();
void MoveBullet2();
void MoveBullet3();
void MoveBullet4();
void MoveBullet5();
void MoveBullet6();
void MoveBullet7();
void MoveBullet8();
void BulletColl();
void CreateShipBullets();
void CheckIfThereIsAShipBulletToDrop();
void GameOver();
void MoveShipBullet1();
void MoveShipBullet2();
void MoveShipBullet3();
void MoveShipBullet4();
void MoveShipBullet5();
void MoveShipBullet6();
void MoveShipBullet7();
void MoveShipBullet8();
void MoveShipBullet9();
void MoveShipBullet10();
void ShipBulletCollCheck();
void CreateWalls();
void adc_init();
void ProcessBulletCurve();


//Globals
int score;
int lives;
int level;
bool gameover;
bool levelSelected;
bool horizontalalienDirection;
bool vertialalienDirection;

bool rowonedirection;
bool rowtwodirection;
bool rowthreedirection;

int shipX;
int shipY;

int alientoprowX;
int alientoprowY;


int alienmiddlerowX;
int alienmiddlerowY;


int alienbottomrowX;
int alienbottomrowY;

//Starting location of tick sprite for start menu 
int selectX;
int selectY;


//rand number to pick what alien to drop a bullet from 
int alienNumber;

// Create a ship
Sprite ship;

// Create alien sprite 
Sprite alien[16];

// Create Level select sprite
Sprite select;

// Create the bullet sprites for the aliens to drop
Sprite alienbullet[9];

// Create the bullet sprites for the space ship to fire 
Sprite shipbullet[11];

//Create the left wall sprites
Sprite Wall[43];


//the bitmap for the ship
unsigned char bm[] = {
	0b00011000,
	0b00011000,
	0b00100100,
	0b10100101,
	0b11000011,
	0b10011001,
	0b10100101,
	0b11000011
};

//bit map for the aliens
unsigned char bm2[] = {
	0b11110000,
	0b01100000,
	0b01100000,
	0b10010000
};

//bit map for the select icon
unsigned char bm3[] = {
	0b00000000,
	0b00000000,
	0b00000001,
	0b00000010,
	0b00000100,
	0b00001000,
	0b10010000,
	0b01100000
};

//bit map for the bullets
unsigned char bm4[] = {
	0b10000000
};




void SetupStartingValues() {
	score = 0;
	lives = 3;
	gameover = false;
	horizontalalienDirection = true;
	vertialalienDirection = true;
	rowonedirection = true; //heading left
	rowtwodirection = false; //heading right
	rowthreedirection = true; //heading left
	shipX = LCD_X / 2;
	shipY = LCD_Y - 10;

	alientoprowX = 20;
	alientoprowY = 6;

	alienmiddlerowX = 20;
	alienmiddlerowY = 11;

	alienbottomrowX = 20;
	alienbottomrowY = 16;
}

// read adc value
uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with '7' will always keep the value
	// of 'ch' between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8) | ch;     // clears the bottom 3 bits before ORing

									 // start single conversion
									 // write '1' to ADSC
	ADCSRA |= (1 << ADSC);

	// wait for conversion to complete
	// ADSC becomes '0' again
	// till then, run loop continuously
	while (ADCSRA & (1 << ADSC));

	return (ADC);
}

int main(void) {

	InitHardware();

	ChooseLevel();

	EventLoop();

	GameOver();

}//end main

void InitHardware() {
	//set the cpu speed to 8MHz (also make sure to compile at 8MHz)
	set_clock_speed(CPU_8MHz);
	
	//initialize adc and lcd
	adc_init();

	//initialize the LCD
	lcd_init(LCD_DEFAULT_CONTRAST);

	// Initalising the push buttons as inputs
	DDRF &= ~((1 << PF5) | (1 << PF6));

	//set the led for the display to output
	DDRC = 0b10000000;

	//turn the backlight of the screen on so we can game/code at night!
	PORTC = 0b10000000;

	//set up the location of the select icon starting on level 1
	selectX = 50;
	selectY = 8;

	//set level selected to false
	levelSelected = false;

	//set level to something random this will be changed
	level = 5;

	// Setup TIMER1 in "normal" operation mode
	TCCR1B &= ~((1 << WGM12));

	//a prescaler of 64 which is 4x less than 256. a prescaler 256 over flows every 2.075 
	//secs so a prescaler of 64 will over flow aprox every half a second
	//this timer is used to control the aliens moving
	TCCR1B &= ~(1 << CS12);
	TCCR1B |= ((1 << CS11)); //sets the bit
	TCCR1B |= ((1 << CS10));
	// Enable the Timer Overflow Interrupt for TIMER1
	TIMSK1 = (1 << TOIE1);

	//
	// Setup TIMER3 in "normal" operation mode
	TCCR3B &= ~((1 << WGM32));
	// Set the prescaler for TIMER3 so that the clock overflows every ~2.1 seconds
	TCCR3B |= (1 << CS32);
	TCCR3B &= ~((1 << CS31));
	TCCR3B &= ~((1 << CS30));
	// Enable the Timer Overflow Interrupt for TIMER1
	//page 136 and lecture demo for info on how to set this
	TIMSK3 = (1 << TOIE3);

	// Globally enable interrupts
	sei();
}

void EventLoop() {
	//setup all the starting values
	SetupStartingValues();
	CreateSpaceShip();
	CreateAlienBullets();
	CreateAlien();
	CreateShipBullets();
	CreateWalls();

	////draw the ship sprite initially
	clear_screen();
	draw_sprite(&ship);
	show_screen();
	


	while (gameover == false) {
		//Process key press
		ProcessKey();

		//Process the bullet curve
		if (level == 3) {
			ProcessBulletCurve();
		}

		//set bullets that are off the screen to fire mode
		BulletColl();

		//clear any characters that may have been written on the screen
		clear_screen();

		//draw the hud
		DrawHUD();

		////draw the ship sprite
		draw_sprite(&ship);


		//draw the aliens
		for (int i = 1; i <= 15; i++) {
			draw_sprite(&alien[i]);
		}


		//draw all aliens bullets
		for (int i = 1; i <= 8; i++) {
			draw_sprite(&alienbullet[i]);
		}

		//draw all the ships bullets
		for (int i = 1; i <= 10; i++) {
			draw_sprite(&shipbullet[i]);
		}

		//draw the walls if its level 2 or 3
		if (level == 2 || level == 3) {
			for (int i = 1; i <= 42; i++) {
				draw_sprite(&Wall[i]);
			}
		}


		//show the screen
		show_screen();

		//Check to see if the player has lost of won yet
		if (lives < 1 || score > 14) {
			gameover = true;
		}
	}
}

void DrawHUD() {
	//draw score
	draw_string_small(0, 0, "S:");
	draw_int_small(10, 0, score);
	//sprintf(buff1, "%d", score);
	//draw_string_small(10, 0, buff1);

	//draw lives
	draw_string_small(25, 0, "LR:");
	draw_int_small(40, 0, lives);
	//sprintf(buff2, "%d", lives);
	//draw_string_small(40, 0, buff2);

	//draw level
	draw_string_small(50, 0, "LvL:");
	draw_int_small(72, 0, level);
	//sprintf(buff3, "%d", level);
	//draw_string_small(72, 0, buff3);

}

void CreateSpaceShip() {
	//initialise the ship sprite
	init_sprite(&ship, shipX, shipY, 8, 8, bm);
}

void CreateAlien() {
	//draw the top row
	for (int i = 1; i <= 5; i++) {
		init_sprite(&alien[i], alientoprowX, alientoprowY, 4, 4, bm2);
		alientoprowX += 10;
	}

	//draw the middle row of aliens
	for (int i = 6; i <= 10; i++) {
		init_sprite(&alien[i], alienmiddlerowX, alienmiddlerowY, 4, 4, bm2);
		alienmiddlerowX += 10;
	}

	//draw the bottom row of aliens
	for (int i = 11; i <= 15; i++) {
		init_sprite(&alien[i], alienbottomrowX, alienbottomrowY, 4, 4, bm2);
		alienbottomrowX += 10;
	}
}

void CreateShipBullets() {
	//init all the ship bullets on the screen out of the way of everything (top right corner) and make them invisible
	for (int i = 1; i <= 10; i++) {
		init_sprite(&shipbullet[i], 80, 0, 1, 1, bm4);
		shipbullet[i].is_visible = false;
	}
}

void CreateAlienBullets() {
	//init all the bullets on the screen out of the way of everything (top right corner) and make them invisible
	for (int i = 1; i <= 8; i++) {
		init_sprite(&alienbullet[i], 80, 0, 1, 1, bm4);
		alienbullet[i].is_visible = false;
	}
}

void ProcessKey() {
	//stuff for the side scrolly input thingy 
	uint16_t adc_result0, adc_result1;

	//if up
	if (!bit_is_clear(PIND, 1)) {
		//a small delay to wait out the bounce effect
		_delay_ms(100);
		//remain in this state while the pin is in this state
		while ((PIND >> PD1) & 1);
		//FIRE A BULLET
		CheckIfThereIsAShipBulletToDrop();

	}
	if (level == 1) {
		//if port f pin 5 (right button) is 1 (its on/high)
		if (!bit_is_clear(PINF, 5)) {
			//a small delay to wait out the bounce effect
			_delay_ms(100);
			//remain in this state while the pin is in this state
			while ((PINF >> PF5) & 1);
			ship.x = ship.x + 2;
		}
		//if port f pin 6 (left button) is 1 (its on/high)
		if (!bit_is_clear(PINF, 6)) {
			//a small delay to wait out the bounce effect
			_delay_ms(100);
			//remain in this state while the pin is in this state
			while ((PINF >> PF6) & 1);
			if (ship.x > 0) {
				ship.x = ship.x - 2;
			}
		}
	}

	if (level == 2 || level == 3) {
		//read both potentiomenters, but I only use one 
		adc_result0 = adc_read(0);      // read adc value at PA0
		adc_result1 = adc_read(1);      // read adc value at PA1

										// conversion of adc value to position values 	  
		float max_adc = 1023.0;
		long max_lcd_adc = (adc_result1*(long)(LCD_X - 5)) / max_adc;

		char tmp[10];
		itoa(adc_result1, tmp, 10);

		ship.x = max_lcd_adc;
	}
	//keep the space ship inside the game screen
	if (ship.x <= 0) {
		ship.x = 0;
	}
	if (ship.x >= 75) {
		ship.x = 75;
	}
}

void ChooseLevel() {
	init_sprite(&select, selectX, selectY, 8, 8, bm3);
	clear_screen();
	draw_sprite(&select);
	show_screen();
	while (levelSelected == false) {
		ProcessLevel();
		clear_screen();
		draw_sprite(&select);
		draw_string(10, 0, "Pick a Level");
		draw_string(10, 10, "Level 1");
		draw_string(10, 20, "Level 2");
		draw_string(10, 30, "Level 3");		
		show_screen();
	}
}

void ProcessLevel() {
	//if port f pin 5 (right button) is 1 (its on/high)
	if (!bit_is_clear(PINF, 5)) {
		//a small delay to wait out the bounce effect
		_delay_ms(100);
		//remain in this state while the pin is in this state
		while ((PINF >> PF5) & 1);
		if (select.y == 8) {
			select.y = 18; //level 2
			level = 2;
		}
		else if (select.y == 18) {
			select.y = 28; //level 3
			level = 3;
		}
		else if (select.y == 28) {
			select.y = 8; //level 1
			level = 1;
		}
	}
	//if port f pin 6 (left button) is 1 (its on/high)
	if (!bit_is_clear(PINF, 6)) {
		//a small delay to wait out the bounce effect
		_delay_ms(100);
		//remain in this state while the pin is in this state
		while ((PINF >> PF6) & 1);
		levelSelected = true;
	}
}

ISR(TIMER1_OVF_vect) {
	// Interrupt service routine for TIMER1. (around every half a second)  
	MoveAliens();
	// If a bullet is visible it should be moving
	MoveVisibleBullets();
}

ISR(TIMER3_OVF_vect) {
	// Interrupt service routine for TIMER3. (around every 2 secs)
	//pick a random alien to drop a bullet from 
	ChooseWhereToDropBullet();
}

void MoveAliens() {
	//Move the aliens towards the left edge of the screen
	if (level == 1 || level == 2) {
		if (horizontalalienDirection == true) {
			for (int i = 1; i <= 15; i++) {
				alien[i].x = alien[i].x - 1;
			}
		}
	}
	//Move the aliens towards the right edge of the screen
	if (level == 1 || level == 2) {
		if (horizontalalienDirection == false) {
			for (int i = 1; i <= 15; i++) {
				alien[i].x = alien[i].x + 1;
			}
		}
	}

	//moving aliens on level 3
	if (level == 3) {
		//top row movement
		if (rowonedirection == true) {
			for (int i = 1; i <= 5; i++) {
				alien[i].x -= 1;
			}
		}
		else if (rowonedirection == false) {
			for (int i = 1; i <= 5; i++) {
				alien[i].x += 1;
			}
		}

		//middle row movement
		if (rowtwodirection == true) {
			for (int i = 6; i <= 10; i++) {
				alien[i].x -= 1;
			}
		}
		else if (rowtwodirection == false) {
			for (int i = 6; i <= 10; i++) {
				alien[i].x += 1;
			}
		}

		//bottom row movement
		if (rowthreedirection == true) {
			for (int i = 11; i <= 15; i++) {
				alien[i].x -= 1;
			}
		}
		else if (rowthreedirection == false) {
			for (int i = 11; i <= 15; i++) {
				alien[i].x += 1;
			}
		}
	}

	//making aliens bounce off walls for level 3
	if (level == 3) {
		if ((alien[1].is_visible == true && alien[1].x == 0) || (alien[2].is_visible == true && alien[2].x == 0) || (alien[3].is_visible == true && alien[3].x == 0) || (alien[4].is_visible == true && alien[4].x == 0) || (alien[5].is_visible == true && alien[5].x == 0)) {
			rowonedirection = false; //heading right
		}
		if ((alien[1].is_visible == true && alien[1].x == 80) || (alien[2].is_visible == true && alien[2].x == 80) || (alien[3].is_visible == true && alien[3].x == 80) || (alien[4].is_visible == true && alien[4].x == 80) || (alien[5].is_visible == true && alien[5].x == 80)) {
			rowonedirection = true; //heading left
		}
		if ((alien[6].is_visible == true && alien[6].x == 0) || (alien[7].is_visible == true && alien[7].x == 0) || (alien[8].is_visible == true && alien[8].x == 0) || (alien[9].is_visible == true && alien[9].x == 0) || (alien[10].is_visible == true && alien[10].x == 0)) {
			rowtwodirection = false; //heading right
		}
		if ((alien[6].is_visible == true && alien[6].x == 80) || (alien[7].is_visible == true && alien[7].x == 80) || (alien[8].is_visible == true && alien[8].x == 80) || (alien[9].is_visible == true && alien[9].x == 80) || (alien[10].is_visible == true && alien[10].x == 80)) {
			rowtwodirection = true; //heading left
		}
		if ((alien[11].is_visible == true && alien[11].x == 0) || (alien[12].is_visible == true && alien[12].x == 0) || (alien[13].is_visible == true && alien[13].x == 0) || (alien[14].is_visible == true && alien[14].x == 0) || (alien[15].is_visible == true && alien[15].x == 0)) {
			rowthreedirection = false; //heading left
		}
		if ((alien[11].is_visible == true && alien[11].x == 80) || (alien[12].is_visible == true && alien[12].x == 80) || (alien[13].is_visible == true && alien[13].x == 80) || (alien[14].is_visible == true && alien[14].x == 80) || (alien[15].is_visible == true && alien[15].x == 80)) {
			rowthreedirection = true; //heading left
		}
	}

	if (level == 1 || level == 2 || level == 3) {
		//if any of the aliens hit the left edge of the screen they will change direction
		if ((alien[1].x == 0 && alien[1].is_visible == true) || (alien[2].x == 0 && alien[2].is_visible == true) || (alien[3].x == 0 && alien[3].is_visible == true) || (alien[4].x == 0 && alien[4].is_visible == true) || (alien[5].x == 0 && alien[5].is_visible == true) || (alien[6].x == 0 && alien[6].is_visible == true) || (alien[7].x == 0 && alien[7].is_visible == true) || (alien[8].x == 0 && alien[8].is_visible == true) || (alien[9].x == 0 && alien[9].is_visible == true) || (alien[10].x == 0 && alien[10].is_visible == true) || (alien[11].x == 0 && alien[11].is_visible == true) || (alien[12].x == 0 && alien[12].is_visible == true) || (alien[13].x == 0 && alien[13].is_visible == true) || (alien[14].x == 0 && alien[14].is_visible == true) || (alien[15].x == 0 && alien[15].is_visible == true)) {
			horizontalalienDirection = false; //moving right

			if (level == 2 || level == 3) {
				//Move all the aliens down one pixel or up a pixel depending on that value of verticalalienDirection
				if (vertialalienDirection == true) {
					for (int i = 1; i <= 15; i++) {
						alien[i].y += 1;
					}
				}
				if (vertialalienDirection == false) {
					for (int i = 1; i <= 15; i++) {
						alien[i].y -= 1;
					}
				}
			}

			if (level == 2 || level == 3) {
				//if any of the aliens THAT are alive (visible) are half way down the screen set the direction of movement to up
				if ((alien[1].is_visible == true && alien[1].y == LCD_Y / 2) || (alien[2].is_visible == true && alien[2].y == LCD_Y / 2) || (alien[3].is_visible == true && alien[3].y == LCD_Y / 2) || (alien[4].is_visible == true && alien[4].y == LCD_Y / 2) || (alien[5].is_visible == true && alien[5].y == LCD_Y / 2) || (alien[6].is_visible == true && alien[6].y == LCD_Y / 2) || (alien[7].is_visible == true && alien[7].y == LCD_Y / 2) || (alien[8].is_visible == true && alien[8].y == LCD_Y / 2) || (alien[9].is_visible == true && alien[9].y == LCD_Y / 2) || (alien[10].is_visible == true && alien[10].y == LCD_Y / 2) || (alien[11].is_visible == true && alien[11].y == LCD_Y / 2) || (alien[12].is_visible == true && alien[12].y == LCD_Y / 2) || (alien[13].is_visible == true && alien[13].y == LCD_Y / 2) || (alien[14].is_visible == true && alien[14].y == LCD_Y / 2) || (alien[15].is_visible == true && alien[15].y == LCD_Y / 2)) {
					vertialalienDirection = false;
				}
				//if any of the aliens THAT are alive (visible) and hit the top of the screen (just under the score etc) set direction of movemnt to down
				if ((alien[1].is_visible == true && alien[1].y == 7) || (alien[2].is_visible == true && alien[2].y == 7) || (alien[3].is_visible == true && alien[3].y == 7) || (alien[4].is_visible == true && alien[4].y == 7) || (alien[5].is_visible == true && alien[5].y == 7) || (alien[6].is_visible == true && alien[6].y == 7) || (alien[7].is_visible == true && alien[7].y == 7) || (alien[8].is_visible == true && alien[8].y == 7) || (alien[9].is_visible == true && alien[9].y == 7) || (alien[10].is_visible == true && alien[10].y == 7) || (alien[11].is_visible == true && alien[11].y == 7) || (alien[12].is_visible == true && alien[12].y == 7) || (alien[13].is_visible == true && alien[13].y == 7) || (alien[14].is_visible == true && alien[14].y == 7) || (alien[15].is_visible == true && alien[15].y == 7)) {
					vertialalienDirection = true;
				}
			}
		}
		//if any of the aliens hit the right edge of the screen they will change direction
		if ((alien[1].x == 80 && alien[1].is_visible == true) || (alien[2].x == 80 && alien[2].is_visible == true) || (alien[3].x == 80 && alien[3].is_visible == true) || (alien[4].x == 80 && alien[4].is_visible == true) || (alien[5].x == 80 && alien[5].is_visible == true) || (alien[6].x == 80 && alien[6].is_visible == true) || (alien[7].x == 80 && alien[7].is_visible == true) || (alien[8].x == 80 && alien[8].is_visible == true) || (alien[9].x == 80 && alien[9].is_visible == true) || (alien[10].x == 80 && alien[10].is_visible == true) || (alien[11].x == 80 && alien[11].is_visible == true) || (alien[12].x == 80 && alien[12].is_visible == true) || (alien[13].x == 80 && alien[13].is_visible == true) || (alien[14].x == 80 && alien[14].is_visible == true) || (alien[15].x == 80 && alien[15].is_visible == true)) {
			horizontalalienDirection = true; //moving left

			if (level == 2 || level == 3) {
				//Move all the aliens down one pixel or up a pixel depending on that value of verticalalienDirection
				if (vertialalienDirection == true) {
					for (int i = 1; i <= 15; i++) {
						alien[i].y += 1;
					}
				}
				if (vertialalienDirection == false) {
					for (int i = 1; i <= 15; i++) {
						alien[i].y -= 1;
					}
				}
			}

			if (level == 2 || level == 3) {
				//if any of the aliens THAT are alive (visible) are half way down the screen set the direction of movement to up
				if ((alien[1].is_visible == true && alien[1].y == LCD_Y / 2) || (alien[2].is_visible == true && alien[2].y == LCD_Y / 2) || (alien[3].is_visible == true && alien[3].y == LCD_Y / 2) || (alien[4].is_visible == true && alien[4].y == LCD_Y / 2) || (alien[5].is_visible == true && alien[5].y == LCD_Y / 2) || (alien[6].is_visible == true && alien[6].y == LCD_Y / 2) || (alien[7].is_visible == true && alien[7].y == LCD_Y / 2) || (alien[8].is_visible == true && alien[8].y == LCD_Y / 2) || (alien[9].is_visible == true && alien[9].y == LCD_Y / 2) || (alien[10].is_visible == true && alien[10].y == LCD_Y / 2) || (alien[11].is_visible == true && alien[11].y == LCD_Y / 2) || (alien[12].is_visible == true && alien[12].y == LCD_Y / 2) || (alien[13].is_visible == true && alien[13].y == LCD_Y / 2) || (alien[14].is_visible == true && alien[14].y == LCD_Y / 2) || (alien[15].is_visible == true && alien[15].y == LCD_Y / 2)) {
					vertialalienDirection = false;
				}
				//if any of the aliens THAT are alive (visible) and hit the top of the screen (just under the score etc) set direction of movemnt to down
				if ((alien[1].is_visible == true && alien[1].y == 7) || (alien[2].is_visible == true && alien[2].y == 7) || (alien[3].is_visible == true && alien[3].y == 7) || (alien[4].is_visible == true && alien[4].y == 7) || (alien[5].is_visible == true && alien[5].y == 7) || (alien[6].is_visible == true && alien[6].y == 7) || (alien[7].is_visible == true && alien[7].y == 7) || (alien[8].is_visible == true && alien[8].y == 7) || (alien[9].is_visible == true && alien[9].y == 7) || (alien[10].is_visible == true && alien[10].y == 7) || (alien[11].is_visible == true && alien[11].y == 7) || (alien[12].is_visible == true && alien[12].y == 7) || (alien[13].is_visible == true && alien[13].y == 7) || (alien[14].is_visible == true && alien[14].y == 7) || (alien[15].is_visible == true && alien[15].y == 7)) {
					vertialalienDirection = true;
				}

			}
		}
	}
}

void ChooseWhereToDropBullet() {
	//get a random number between 1 - 15 that will be used to pick what alien to drop a bullet from 
	alienNumber = (rand() % 15) + 1;
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		//if the alien is alive still see if there is a free bullet to drop 
		if ((alienNumber == numbercheck) && (alien[i].is_visible == true)) {
			CheckIfThereIsAAlienBulletToDrop();
		}
		numbercheck++;
	}
}

void MoveVisibleBullets() {
	//if an alien bullet is visible make it move down the screen
	for (int i = 1; i <= 8; i++) {
		if (alienbullet[i].is_visible == true) {
			alienbullet[i].y++;
		}
	}
	//if a ship bullet is visible make it move up the screen
	for (int i = 1; i <= 10; i++) {
		if (shipbullet[i].is_visible == true) {
			shipbullet[i].y--;
		}
	}
}

void CheckIfThereIsAAlienBulletToDrop() {
	//If an alienbullet is false its ready to be FIRED AGAIN 
	if (alienbullet[1].is_visible == false) {
		//Move the bullet under the correct alien ship
		MoveBullet1();
		//Pew Pew (Fire the bullet)
		alienbullet[1].is_visible = true;
	}
	else if (alienbullet[2].is_visible == false) {
		MoveBullet2();
		alienbullet[2].is_visible = true;
	}
	else if (alienbullet[3].is_visible == false) {
		MoveBullet3();
		alienbullet[3].is_visible = true;
	}
	else if (alienbullet[4].is_visible == false) {
		MoveBullet4();
		alienbullet[4].is_visible = true;
	}
	else if (alienbullet[5].is_visible == false) {
		MoveBullet5();
		alienbullet[5].is_visible = true;
	}
	else if (alienbullet[6].is_visible == false) {
		MoveBullet6();
		alienbullet[6].is_visible = true;
	}
	else if (alienbullet[7].is_visible == false) {
		MoveBullet7();
		alienbullet[7].is_visible = true;
	}
	else if (alienbullet[8].is_visible == false) {
		MoveBullet8();
		alienbullet[8].is_visible = true;
	}
}

void CheckIfThereIsAShipBulletToDrop() {
	//If a ship bullet is not visible its ready to be fired again
	if (shipbullet[1].is_visible == false) {
		//Move the bullet above the space ship
		MoveShipBullet1();
		//Pew! Pew! (Fire the bullet)
		shipbullet[1].is_visible = true;
	}
	else if (shipbullet[2].is_visible == false) {
		MoveShipBullet2();
		shipbullet[2].is_visible = true;
	}
	else if (shipbullet[3].is_visible == false) {
		MoveShipBullet3();
		shipbullet[3].is_visible = true;
	}
	else if (shipbullet[4].is_visible == false) {
		MoveShipBullet4();
		shipbullet[4].is_visible = true;
	}
	else if (shipbullet[5].is_visible == false) {
		MoveShipBullet5();
		shipbullet[5].is_visible = true;
	}
	else if (shipbullet[6].is_visible == false) {
		MoveShipBullet6();
		shipbullet[6].is_visible = true;
	}
	else if (shipbullet[7].is_visible == false) {
		MoveShipBullet7();
		shipbullet[7].is_visible = true;
	}
	else if (shipbullet[8].is_visible == false) {
		MoveShipBullet8();
		shipbullet[8].is_visible = true;
	}
	else if (shipbullet[9].is_visible == false) {
		MoveShipBullet9();
		shipbullet[9].is_visible = true;
	}
	else if (shipbullet[10].is_visible == false) {
		MoveShipBullet10();
		shipbullet[10].is_visible = true;
	}
}

void MoveBullet1() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[1].x = alien[i].x + 2;
			alienbullet[1].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet2() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[2].x = alien[i].x + 2;
			alienbullet[2].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet3() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[3].x = alien[i].x + 2;
			alienbullet[3].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet4() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[4].x = alien[i].x + 2;
			alienbullet[4].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet5() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[5].x = alien[i].x + 2;
			alienbullet[5].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet6() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[6].x = alien[i].x + 2;
			alienbullet[6].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet7() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[7].x = alien[i].x + 2;
			alienbullet[7].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void MoveBullet8() {
	int numbercheck = 1;
	for (int i = 1; i <= 15; i++) {
		if (alienNumber == numbercheck) {
			alienbullet[8].x = alien[i].x + 2;
			alienbullet[8].y = alien[i].y + 4;
		}
		numbercheck++;
	}
}

void BulletColl() {
	//if any alien bullets are off the bottom edge of the screen set their visibility to false so they 
	//can be fired again
	for (int i = 1; i <= 8; i++) {
		if (alienbullet[i].y >= 46) {
			alienbullet[i].is_visible = false;
		}
	}

	//if any ship bullets are past the top of the screen (just under the score etc) set their visibility to false so they can be fired again
	for (int i = 1; i <= 10; i++) {
		if (shipbullet[i].y <= 7) {
			shipbullet[i].is_visible = false;
			//I HAVE TO MOVE THEM OUT OF THE WAY HERE AS THEY ARE STILL KILLING ALIENS 
			//move the bullet out of the way so the invis bullets that hit the roof dont keep killing aliens as they fly over them 
			shipbullet[i].y = 0;
			shipbullet[i].x = 80;
		}
	}

	//check for alien bullets hitting the ship 
	for (int i = 1; i <= 8; i++) {
		if ((alienbullet[i].y >= ship.y) && ((alienbullet[i].x >= ship.x) && (alienbullet[i].x <= ship.x + 7))) {
			if (alienbullet[i].is_visible == true) {
				lives--;
			}
			alienbullet[i].is_visible = false;
		}
	}

	//check for alien bullets hitting the horizontal walls
	if (level == 2 || level == 3) {
		for (int i = 1; i <= 42; i++) {
			if ((alienbullet[1].y == Wall[i].y) && (alienbullet[1].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[1].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[2].y == Wall[i].y) && (alienbullet[2].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[2].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[3].y == Wall[i].y) && (alienbullet[3].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[3].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[4].y == Wall[i].y) && (alienbullet[4].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[4].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[5].y == Wall[i].y) && (alienbullet[5].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[5].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[6].y == Wall[i].y) && (alienbullet[6].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[6].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[7].y == Wall[i].y) && (alienbullet[7].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[7].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
			if ((alienbullet[8].y == Wall[i].y) && (alienbullet[8].x == Wall[i].x)) {
				if (Wall[i].is_visible == true) {
					alienbullet[8].is_visible = false;
					Wall[i].is_visible = false;
				}
			}
		}
	}

	//check for a ship bullet hitting an alien
	ShipBulletCollCheck();
}

void GameOver() {
	clear_screen();
	if (score == 15) {
		draw_string(20, 15, "Game Over");
		draw_string(20, 30, "You Win");
	}
	if (lives == 0) {
		draw_string(20, 15, "Game Over");
		draw_string(20, 30, "You Lose");
	}	
	show_screen();
}

void MoveShipBullet1() {
	shipbullet[1].x = ship.x + 4;
	shipbullet[1].y = ship.y - 1;
}

void MoveShipBullet2() {
	shipbullet[2].x = ship.x + 4;
	shipbullet[2].y = ship.y - 1;
}

void MoveShipBullet3() {
	shipbullet[3].x = ship.x + 4;
	shipbullet[3].y = ship.y - 1;
}

void MoveShipBullet4() {
	shipbullet[4].x = ship.x + 4;
	shipbullet[4].y = ship.y - 1;
}

void MoveShipBullet5() {
	shipbullet[5].x = ship.x + 4;
	shipbullet[5].y = ship.y - 1;
}

void MoveShipBullet6() {
	shipbullet[6].x = ship.x + 4;
	shipbullet[6].y = ship.y - 1;
}

void MoveShipBullet7() {
	shipbullet[7].x = ship.x + 4;
	shipbullet[7].y = ship.y - 1;
}

void MoveShipBullet8() {
	shipbullet[8].x = ship.x + 4;
	shipbullet[8].y = ship.y - 1;
}

void MoveShipBullet9() {
	shipbullet[9].x = ship.x + 4;
	shipbullet[9].y = ship.y - 1;
}

void MoveShipBullet10() {
	shipbullet[10].x = ship.x + 4;
	shipbullet[10].y = ship.y - 1;
}

void ShipBulletCollCheck() {
	for (int i = 1; i <= 15; i++) {
		if ((alien[i].is_visible == true) && ((shipbullet[1].y <= alien[i].y + 4) && (shipbullet[1].y >= alien[i].y)) && ((shipbullet[1].x >= alien[i].x) && (shipbullet[1].x <= alien[i].x + 3))) {
			if (shipbullet[1].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[1].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[1].y = 0;
			shipbullet[1].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[2].y <= alien[i].y + 4) && (shipbullet[2].y >= alien[i].y)) && ((shipbullet[2].x >= alien[i].x) && (shipbullet[2].x <= alien[i].x + 3))) {
			if (shipbullet[2].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[2].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[2].y = 0;
			shipbullet[2].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[3].y <= alien[i].y + 4) && (shipbullet[3].y >= alien[i].y)) && ((shipbullet[3].x >= alien[i].x) && (shipbullet[3].x <= alien[i].x + 3))) {
			if (shipbullet[3].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[3].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[3].y = 0;
			shipbullet[3].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[4].y <= alien[i].y + 4) && (shipbullet[4].y >= alien[i].y)) && ((shipbullet[4].x >= alien[i].x) && (shipbullet[4].x <= alien[i].x + 3))) {
			if (shipbullet[4].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[4].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[4].y = 0;
			shipbullet[4].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[5].y <= alien[i].y + 4) && (shipbullet[5].y >= alien[i].y)) && ((shipbullet[5].x >= alien[i].x) && (shipbullet[5].x <= alien[i].x + 3))) {
			if (shipbullet[5].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[5].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[5].y = 0;
			shipbullet[5].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[6].y <= alien[i].y + 4) && (shipbullet[6].y >= alien[i].y)) && ((shipbullet[6].x >= alien[i].x) && (shipbullet[6].x <= alien[i].x + 3))) {
			if (shipbullet[6].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[6].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[6].y = 0;
			shipbullet[6].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[7].y <= alien[i].y + 4) && (shipbullet[7].y >= alien[i].y)) && ((shipbullet[7].x >= alien[i].x) && (shipbullet[7].x <= alien[i].x + 3))) {
			if (shipbullet[7].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[7].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[7].y = 0;
			shipbullet[7].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[8].y <= alien[i].y + 4) && (shipbullet[8].y >= alien[i].y)) && ((shipbullet[8].x >= alien[i].x) && (shipbullet[8].x <= alien[i].x + 3))) {
			if (shipbullet[8].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[8].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[8].y = 0;
			shipbullet[8].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[9].y <= alien[i].y + 4) && (shipbullet[9].y >= alien[i].y)) && ((shipbullet[9].x >= alien[i].x) && (shipbullet[9].x <= alien[i].x + 3))) {
			if (shipbullet[9].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[9].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[9].y = 0;
			shipbullet[9].x = 80;
		}
		if ((alien[i].is_visible == true) && ((shipbullet[10].y <= alien[i].y + 4) && (shipbullet[10].y >= alien[i].y)) && ((shipbullet[10].x >= alien[i].x) && (shipbullet[10].x <= alien[i].x + 3))) {
			if (shipbullet[10].is_visible == true) {
				score++;
			}
			alien[i].is_visible = false;
			shipbullet[10].is_visible = false;
			//move the bullet out of the way so the invis bullet does not keep killing aliens as they fly over it 
			shipbullet[10].y = 0;
			shipbullet[10].x = 80;
		}

		//Check if ship bullets have hit the horiz walls
		if (level == 2 || level == 3) {
			for (int i = 1; i <= 42; i++) {
				if ((shipbullet[1].y == Wall[i].y) && (shipbullet[1].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[1].y = 0;
						shipbullet[1].x = 80;
						shipbullet[1].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[2].y == Wall[i].y) && (shipbullet[2].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[2].y = 0;
						shipbullet[2].x = 80;
						shipbullet[2].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[3].y == Wall[i].y) && (shipbullet[3].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[3].y = 0;
						shipbullet[3].x = 80;
						shipbullet[3].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[4].y == Wall[i].y) && (shipbullet[4].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[4].y = 0;
						shipbullet[4].x = 80;
						shipbullet[4].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[5].y == Wall[i].y) && (shipbullet[5].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[5].y = 0;
						shipbullet[5].x = 80;
						shipbullet[5].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[6].y == Wall[i].y) && (shipbullet[6].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[6].y = 0;
						shipbullet[6].x = 80;
						shipbullet[6].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[7].y == Wall[i].y) && (shipbullet[7].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[7].y = 0;
						shipbullet[7].x = 80;
						shipbullet[7].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[8].y == Wall[i].y) && (shipbullet[8].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[8].y = 0;
						shipbullet[8].x = 80;
						shipbullet[8].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[9].y == Wall[i].y) && (shipbullet[9].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[9].y = 0;
						shipbullet[9].x = 80;
						shipbullet[9].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
				if ((shipbullet[10].y == Wall[i].y) && (shipbullet[10].x == Wall[i].x)) {
					if (Wall[i].is_visible == true) {
						//move the ship bullet out of the way so it does not coll with anything 
						shipbullet[10].y = 0;
						shipbullet[10].x = 80;
						shipbullet[10].is_visible = false;
						Wall[i].is_visible = false;
					}
				}
			}
		}
	}
}

void CreateWalls() {
	int firstRowOfWall1X = 15;
	int firstRowOfWall2X = (LCD_X / 2) - 5;
	int firstRowOfWall3X = (LCD_X - 24);
	

	//left wall
	for (int i = 1; i <= 7; i++) {
		init_sprite(&Wall[i], firstRowOfWall1X, 35, 1, 1, bm4);
		firstRowOfWall1X++;
	}
	firstRowOfWall1X = 15;
	for (int i = 8; i <= 14; i++) {
		init_sprite(&Wall[i], firstRowOfWall1X, 34, 1, 1, bm4);
		firstRowOfWall1X++;
	}

	//middle wall 
	for (int i = 15; i <= 21; i++) {
		init_sprite(&Wall[i], firstRowOfWall2X, 35, 1, 1, bm4);
		firstRowOfWall2X++;
	}
	firstRowOfWall2X = (LCD_X / 2 - 5);
	for (int i = 22; i <= 28; i++) {
		init_sprite(&Wall[i], firstRowOfWall2X, 34, 1, 1, bm4);
		firstRowOfWall2X++;
	}

	//right wall 
	for (int i = 29; i <= 35; i++) {
		init_sprite(&Wall[i], firstRowOfWall3X, 35, 1, 1, bm4);
		firstRowOfWall3X++;
	}
	firstRowOfWall3X = (LCD_X - 24);
	for (int i = 36; i <= 42; i++) {
		init_sprite(&Wall[i], firstRowOfWall3X, 34, 1, 1, bm4);
		firstRowOfWall3X++;
	}


}

// initialize adc
void adc_init()
{
	// AREF = AVcc
	ADMUX = (1 << REFS0);

	// ADC Enable and pre-scaler of 128
	// 8000000/128 = 62500
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void ProcessBulletCurve() {
	//stuff for the side scrolly input thingy 
	uint16_t addc_result0, addc_result1;

	//read both potentiomenters, but I only use one 
	addc_result0 = adc_read(0);      // read adc value at PA0
	addc_result1 = adc_read(1);      // read adc value at PA1

	// conversion of adc value to position values 	  
	float maxx_adc = 1023.0;
	long maxx_lcd_adc = (addc_result0*(long)(LCD_X - 5)) / maxx_adc;


	char tmpp[10];
	itoa(addc_result0, tmpp, 10);
	//need more code here to start the bullet off infront of the ship 

	shipbullet[1].x = maxx_lcd_adc + 4;
	shipbullet[2].x = maxx_lcd_adc + 4;
	shipbullet[3].x = maxx_lcd_adc + 4;
	shipbullet[4].x = maxx_lcd_adc + 4;
	shipbullet[5].x = maxx_lcd_adc + 4;
	shipbullet[6].x = maxx_lcd_adc + 4;
	shipbullet[7].x = maxx_lcd_adc + 4;
	shipbullet[8].x = maxx_lcd_adc + 4;
	shipbullet[9].x = maxx_lcd_adc + 4;
	shipbullet[10].x = maxx_lcd_adc + 4;
}



