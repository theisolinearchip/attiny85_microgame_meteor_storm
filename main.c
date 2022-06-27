#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h> 
#include <stdlib.h>
#include <avr/pgmspace.h> // ssd1306

#include "libs/i2c/i2c_primary.c"
#include "libs/ssd1306/ssd1306_attiny85.c"

// PB2 & PB0 reserved for SCL and SDA

#define PIN_BUTTON_A 		PB1
#define PIN_BUTTON_B 		PB3
#define PIN_BUTTON_C 		PB4

#define is_button_pressed(button) ((!(PINB & (1 << button))) ? 1 : 0)

// GAME STUFF

#define PLAYER_HEIGHT		4 		// 2 pages
#define PLAYER_WIDTH 		8
#define PLAYER_INITIAL_Y	40

#define OBSTACLE_HEIGHT		8
#define OBSTACLE_WIDTH		8
#define OBSTACLE_SPEED		3

#define MAX_OBSTACLES 		6

#define NUMBER_WIDTH 		6

char cursor_x = 0;
char cursor_y = 0;
char cursor_offset_y = 0;

char player_x =  SSD1306_WIDTH / 5; // SSD1306_WIDTH / 2 - PLAYER_WIDTH / 2;
char player_y = PLAYER_INITIAL_Y;
char player_acc = 0;

int obstacles[MAX_OBSTACLES*2]; // x / y (x2!)

char button_pressed_last_step = 0;
char title = 1;

char command_horizontal_mode[2] = { SSD1306_MEMORYMODE, 0x00 };
char command_page_mode[2] = { SSD1306_MEMORYMODE, 0x02 };

int score = 0;
int max_score = 0;
int seed = 0;

// font from here: https://github.com/andyhighnumber/Attiny-Arduino-Games/blob/master/BatBonanzaAttinyArcade/font6x8AJ.h
char numbers[60] = {
	0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
	0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, // 1
	0x00, 0x42, 0x61, 0x51, 0x49, 0x46, // 2
	0x00, 0x21, 0x41, 0x45, 0x4B, 0x31, // 3
	0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, // 4
	0x00, 0x27, 0x45, 0x45, 0x45, 0x39, // 5
	0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
	0x00, 0x01, 0x71, 0x09, 0x05, 0x03, // 7
	0x00, 0x36, 0x49, 0x49, 0x49, 0x36, // 8
	0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, // 9
};

void clean_screen() {
	for (int i = 0; i < 8; i++) {
		ssd1306_send_single_command(0xB0 | i);
		ssd1306_send_multiple_equal_data(128, 0x00);
	}
}

void set_pos(char x, char y) {

	cursor_x = x;
	cursor_y = y;

	cursor_offset_y = y % 8;

	// pages with 8 height, so first aim for the page (0xB{page})
	ssd1306_send_single_command(0xB0 | (y / 8));

	// set column (low nibble, 0x0{nibble}; high nibble, 0x1{nibble})
	ssd1306_send_single_command(x & 0x0F);
	ssd1306_send_single_command(0x10 | (x >> 4));

}

static void draw_score(int s, char page) {
	char tmp = s;
	char _x = SSD1306_WIDTH - NUMBER_WIDTH; // right
	do {
		ssd1306_send_single_command(0xB0 | page);
		ssd1306_send_single_command(_x & 0x0F);
		ssd1306_send_single_command(0x10 | (_x >> 4));

		for (int i = 0; i < NUMBER_WIDTH; i++) {
			ssd1306_send_single_data(numbers[ (tmp % 10) * NUMBER_WIDTH + i]);
		}

		_x -= NUMBER_WIDTH;
		tmp /= 10;
	} while (tmp > 0);
}

static void draw_title() {
	set_pos(0, 0);
	ssd1306_send_multiple_commands(2, command_horizontal_mode); // image stored on horizontal mode
	ssd1306_send_progmem_multiple_data(title_image_length, title_image);
}

static void draw_player() {

	set_pos(player_x, player_y);
	char w = 0; // width drawn
	char r = PLAYER_HEIGHT; // remaining height
	while (w < PLAYER_WIDTH && ((cursor_x + w) < SSD1306_WIDTH)) {
		ssd1306_send_single_data(0x0F << cursor_offset_y);
		w++;
	}
	r -= (SSD1306_PAGESIZE - cursor_offset_y); // remaining height pixels to draw (since height is > 1 page, next one will always be drawn)
	if (r > 0) {
		w = 0;
		set_pos(player_x, player_y + (SSD1306_PAGESIZE - cursor_offset_y)); // + SSD1306_PAGESIZE); // advance for the next (next) page
		char pp = 1;
		while (--r > 0) pp |= (pp << 1);
		while (w < PLAYER_WIDTH && ((cursor_x + w) < SSD1306_WIDTH)) {
			ssd1306_send_single_data(pp);
			w++;
		}
	}

}

static void draw_obstacles() {
	char w;
	char r; // remaining height
	for (int i = 0; i < MAX_OBSTACLES*2; i = i+2) {
		if (obstacles[i] + OBSTACLE_WIDTH < 0 || obstacles[i] >= SSD1306_WIDTH) continue; // only draw the ones visible in the canvas

		set_pos(obstacles[i], obstacles[i+1]);
		w = 0; // width drawn
		r = OBSTACLE_HEIGHT;
		while (w < OBSTACLE_WIDTH && ((cursor_x + w) < SSD1306_WIDTH)) {
			ssd1306_send_single_data(0xFF << cursor_offset_y);
			w++;
		}
		r -= (SSD1306_PAGESIZE - cursor_offset_y);
		if (r > 0) {
			w = 0;
			set_pos(obstacles[i], obstacles[i+1] + (SSD1306_PAGESIZE - cursor_offset_y)); // advance for the next page
			char pp = 1;
			while (--r > 0) pp |= (pp << 1);
			while (w < OBSTACLE_WIDTH && ((cursor_x + w) < SSD1306_WIDTH)) {
				ssd1306_send_single_data(pp);
				w++;
			}
		}
	}
}

static void draw_borders() {
	set_pos(0, 0);
	ssd1306_send_multiple_equal_data(127, 0x01); // top
	set_pos(0, SSD1306_HEIGHT-1);
	ssd1306_send_multiple_equal_data(127, 0x80); // bot
}

void init_game() {

	srand(seed);

	ssd1306_send_multiple_commands(2, command_page_mode);
	title = 0;

	max_score = score > max_score ? score : max_score;
	score = 0;

	player_x = SSD1306_WIDTH / 5;
	player_y = PLAYER_INITIAL_Y;

	for (int i = 0; i < MAX_OBSTACLES*2; i = i+2) {
		obstacles[i] = SSD1306_WIDTH + rand() % 50 + (10 * i);
		obstacles[i+1] = rand() % (SSD1306_HEIGHT - OBSTACLE_HEIGHT); // y
	}
}

static void check_movement() {
	if (is_button_pressed(PIN_BUTTON_A)) {
		if (button_pressed_last_step) {
			button_pressed_last_step = 0;
			player_acc = 0;
		} else {
			if (player_acc-- < -5) player_acc = -5;
		}
	} else {
		if (button_pressed_last_step) {
			if (player_acc++ > 5) player_acc = 5;
		} else {
			player_acc = 0;
			button_pressed_last_step = 1;
		}
	}
}

static void move_player() {
	player_y += player_acc/2;
	if (player_y < 0) player_y = 0;
	else if (player_y + PLAYER_HEIGHT >= SSD1306_HEIGHT) player_y = SSD1306_HEIGHT - PLAYER_HEIGHT;
}

static char check_collisions() {
	// check player collisions with borders and obstacles

	if (player_y <= 0 || player_y + PLAYER_HEIGHT >= SSD1306_HEIGHT) {
		return 1;
	}

	for (int i = 0; i < MAX_OBSTACLES * 2; i = i+2) { // just checking for the edges will do the trick here
		if ( (
			(obstacles[i] >= player_x && obstacles[i] <= player_x + PLAYER_WIDTH) ||
			(obstacles[i] + OBSTACLE_WIDTH >= player_x && obstacles[i] + OBSTACLE_WIDTH <= player_x + PLAYER_WIDTH)
		 ) && (
		 	(obstacles[i+1] >= player_y && obstacles[i+1] <= player_y + PLAYER_HEIGHT) ||
			(obstacles[i+1] + OBSTACLE_HEIGHT >= player_y && obstacles[i+1] + OBSTACLE_HEIGHT <= player_y + PLAYER_HEIGHT)
		) ) {
			return 1;
		}
	}

	return 0;
}

// move obstacles from right to left; if they're at the end, reposition at the beginning
// (use some extra padding on X position to give them some "delay")
void move_obstacles() {

	for (int i = 0; i < MAX_OBSTACLES*2; i = i+2) {
		obstacles[i] -= OBSTACLE_SPEED;

		if (obstacles[i] < 0) {
			// reposition + score
			obstacles[i] = SSD1306_WIDTH + rand() % 50;
			obstacles[i+1] = rand() % (SSD1306_HEIGHT - OBSTACLE_HEIGHT); // y

			score++;
		}
	}
}

void set_game_over() {

	ssd1306_send_single_command(SSD1306_INVERTDISPLAY);
	_delay_ms(1000);
	ssd1306_send_single_command(SSD1306_NORMALDISPLAY);

	title = 1;
	draw_title();
}

int main() {

	_delay_ms(200);

	DDRB &= ~(1 << PIN_BUTTON_A);
	PORTB |= (1 << PIN_BUTTON_A); // activate pull-up (input)
	DDRB &= ~(1 << PIN_BUTTON_B);
	PORTB |= (1 << PIN_BUTTON_B); // activate pull-up (input)
	DDRB &= ~(1 << PIN_BUTTON_C);
	PORTB |= (1 << PIN_BUTTON_C); // activate pull-up (input)

	i2c_init();

	ssd1306_init();
	_delay_ms(500);

	draw_title();
	while(is_button_pressed(PIN_BUTTON_A)); // avoid auto-start (init the game with A button being pressed)

	char col = 0;

	while(1) {

		if (title) {

			seed++; // overflow if you want, we're just aiming for a more or less unpredictable seed

			// init screen must be drawn before
			if (is_button_pressed(PIN_BUTTON_A)) init_game();

		} else {

			// LOGIC 

			// check player buttons and position
			check_movement();

			move_player();

			move_obstacles();

			col = check_collisions();

			// DRAW

			clean_screen();

			draw_borders();

			draw_player();

			draw_obstacles();

			draw_score(score, 0);							// top page
			if (max_score > 0) draw_score(max_score, 7);	// bottom page

			// GAME OVER?
			if (col) {
				set_game_over();
			}

		}

		_delay_ms(10);
	}

}