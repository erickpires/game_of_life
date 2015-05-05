#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "game_of_life.c"

//TODO: Change matrix, board, etc. to world
//TODO: Implement getstr so the directional keys can be used (See TODO below)
//TODO: Create a default save directory and search the files in there while loading.
//      The user should be able to choose the file using the up/down keys
//TODO: Handle change of terminal size events
//TODO: Use Unicode
//TODO: Use stdint (P.S.: This may cause problems in save/load game to file)
//TODO: Count generations
//TODO: The boundaries of the board are creating some forms that shouldn't exist. 
//      (e.g. when a glider reaches the end of the board). This should be handed more
//      clearer.
//README: Should the genarations be saved to the file?


#define GAME_DELAY 100000

#define COLOR_DEAD_CELL 1
#define COLOR_ALIVE_CELL 2
#define COLOR_MODIFIED_ALIVE_CELL 3
#define COLOR_MODIFIED_DEAD_CELL 4

#define COLOR_NORMAL_TEXT COLOR_DEAD_CELL
#define COLOR_INVERTED_TEXT 5


//README: This shouldn't be a global
char* text_buffer;

int init_curses(WINDOW** main_win, int* oldcur){
	*main_win = initscr();
	start_color();

	//TODO: Theses modes should be defined constants
	init_pair(COLOR_DEAD_CELL, COLOR_WHITE, COLOR_BLACK);
	init_pair(COLOR_ALIVE_CELL, COLOR_BLACK, COLOR_WHITE);
	init_pair(COLOR_MODIFIED_ALIVE_CELL, COLOR_WHITE, COLOR_BLUE);
	init_pair(COLOR_MODIFIED_DEAD_CELL, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_INVERTED_TEXT, COLOR_BLUE, COLOR_WHITE);

	*oldcur = curs_set(0);
	noecho();
	cbreak();	

	return 1;
}

void misc_inits(){
	srand(time(NULL));
	text_buffer = (char*) malloc(COLS * sizeof(char) + 1);
}

void wait_for_keypress() {
	timeout(-1);
	getch();
	//TODO: Use previous timeout
	timeout(0);
	//README: Maybe flush stdin here
}

void display_text(char* text, ...){
	va_list args;
	va_start(args, text);
	vsnprintf(text_buffer, COLS + 1, text, args);
	va_end(args);

	move(LINES -1, 0);
	attron(COLOR_PAIR(COLOR_INVERTED_TEXT));
	printw(text_buffer);
}

//TODO: Create a helper function to use var_args here too
void display_text_and_block(char* text) {
	char* continue_msg = "(Press any key to continue) ";
	char* msg = (char*) alloca(strlen(text) + strlen(continue_msg) + 1);
	strcpy(msg, text);
	strcat(msg, continue_msg);

	display_text(msg);
	wait_for_keypress();
}

void clear_text(){
	attron(COLOR_PAIR(1));
	for(int col = 0; col < COLS; col++){
		move(LINES - 1, col);
		addch(' ');
	}
}

void validate_and_apply(game_state* state, int* pos_x, int* pos_y,
						int new_pos_x, int new_pos_y){

	if(new_pos_x >= 0 && new_pos_x < state->cols)
		*pos_x = new_pos_x;
	if(new_pos_y >= 0 && new_pos_y < state->lines)
		*pos_y = new_pos_y;

}

void draw_game_without_refresh(game_state* state, int show_generations){
	for(int line = 0; line < state->lines; line++){
		for(int col = 0; col < state->cols; col++){
			move(line, col);
			int is_current_cell_alive = is_cell_alive(state, line, col);
			if(is_current_cell_alive)
				attron(COLOR_PAIR(COLOR_ALIVE_CELL));
			else
				attron(COLOR_PAIR(COLOR_DEAD_CELL));
			
			addch(' ');
			
		}
	}

	if(show_generations){
		clear_text();
		display_text("Generation: %d", state->generations);
	}
}

void draw_game(game_state* state){
	draw_game_without_refresh(state, FALSE);

	refresh();
}

void draw_game_with_generations(game_state* state){
	draw_game_without_refresh(state, TRUE);

	refresh();
}

void draw_game_edit_mode(game_state* state, int line, int col){
	draw_game_without_refresh(state, FALSE);

	move(line, col);
	if(is_cell_alive(state, line, col))
		attron(COLOR_PAIR(COLOR_MODIFIED_ALIVE_CELL));
	else 
		attron(COLOR_PAIR(COLOR_MODIFIED_DEAD_CELL));

	addch(' '); 

	refresh();
}

void step_game(game_state* state){
	update_game_state(state);

	draw_game_with_generations(state);
}

void loop_game(game_state* state){
	timeout(0);

	while(TRUE){
		int c = getch();
		if(c == 'q')
			return;

		update_game_state(state);
		draw_game_with_generations(state);
		usleep(GAME_DELAY);
	}
}

void edit_game(game_state* state){
	display_text("  Edit Mode  ");

	timeout(-1);
	//README: Maybe change these variables to line and col.
	//		  Or abstract them into a struct.
	//        Or refactor the model code?
	int pos_x = 0;
	int pos_y = 0;

	int has_world_been_modified = 0;

	while(TRUE){
		draw_game_edit_mode(state, pos_y, pos_x);
		
		int input_char = getch();

		switch(input_char){
			case 'q':
				return;
			case 'a':
				set_cell_at_pos(state, pos_y, pos_x, ALIVE);
				break;
			case 'd':
				set_cell_at_pos(state, pos_y, pos_x, DEAD);
				break;
			case 't':
				toggle_cell_at_pos(state, pos_y, pos_x);
				break;
			case 27: //Escape char
				input_char = getch();
				if(input_char != 91) //Expected char
					break;

				input_char = getch();
				int new_pos_x = pos_x;
				int new_pos_y = pos_y;

				switch (input_char){
					case 68: //Left
						new_pos_x--;
						break;
					case 65: //Up
						new_pos_y--;
						break;
					case 67: //Right
						new_pos_x++;
						break;
					case 66: //Down
						new_pos_y++;
						break;
					default:	
						break;
				}
				validate_and_apply(state, &pos_x, &pos_y, new_pos_x, new_pos_y);
				break;
			default:
				break;
		}
	}
}

void prompt_filename(game_state* state, int oldcur, char* buffer){
	echo();
	curs_set(oldcur);

	char* msg = "File name:";
	clear_text();
	display_text(msg);
	draw_game(state);

	move(LINES - 1, strlen(msg) + 1);
	attron(COLOR_PAIR(COLOR_NORMAL_TEXT));
	getstr(buffer);

	noecho();
	curs_set(0);
}

int save_game_to_file(game_state* state, char* filename){
	FILE* output_file = fopen(filename, "wb");

	if(!output_file) return FALSE;

	fwrite(&(state->lines), sizeof(state->lines), 1, output_file);
	fwrite(&(state->cols), sizeof(state->cols), 1, output_file);

	int n_cells = state->lines * state->cols;
	fwrite(state->current_matrix, sizeof(char), n_cells, output_file);

	fclose(output_file);
	return TRUE;
}

int load_game_from_file(game_state* state, char* filename){
	FILE* input_file = fopen(filename, "rb");

	if(!input_file) return FALSE;

	int lines_in_file;
	int cols_in_file;

	fread(&lines_in_file, sizeof(lines_in_file), 1, input_file);
	fread(&cols_in_file, sizeof(cols_in_file), 1, input_file);

	//TODO: Make it user friendly.
	//File is too large. Clipping
	if(lines_in_file > state->lines){
		lines_in_file = state->lines;
	}

	int cells_to_ignore = 0;
	if(cols_in_file > state->cols){
		cells_to_ignore = cols_in_file - state->cols;
		cols_in_file = state->cols;
	}

	//TODO: Handle the case wherein the file is smaller than the game_state
	//      Maybe center the file world

	set_world_to_value(state, DEAD);

	char value;
	char* cells_ptr = state->current_matrix;
	for(int line = 0; line < lines_in_file; line++){
		fread(cells_ptr, sizeof(char), cols_in_file, input_file);

		if(cells_to_ignore){
			fseek(input_file, cells_to_ignore * sizeof(char), SEEK_CUR);
		}
		
		cells_ptr += state->cols;
	}

	clear_generations(state);
	fclose(input_file);

	return TRUE;
}

void prompt_and_save_game_to_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(state, oldcur, buffer);

	if(!save_game_to_file(state, buffer))
		display_text_and_block(" Couldn't save file. ");
}

void prompt_and_load_game_from_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(state, oldcur, buffer);

	if(!load_game_from_file(state, buffer))
		display_text_and_block(" Couldn't open file. ");
}

int main(int argc, char** argv){

	int running = TRUE;
	game_state gameState;

	//README: Maybe bundle these variables in a ncurses_context
	WINDOW* main_win;
	int oldcur;

	if(!init_curses(&main_win, &oldcur))
		return -1;	
	
	misc_inits();

	//Can't exit without endwin. Just skip the while-loop
	if(!init_game_state(&gameState, COLS, LINES - 1))
		running = FALSE;
	

	while(running){
		clear_text();
		draw_game(&gameState);

		timeout(-1);
		int c = getch();

		switch(c) {
			case 'r':
				randomize_game(&gameState);
				clear_generations(&gameState);
				break;
			case ' ':
				step_game(&gameState);
				break;
			case 'p':
				loop_game(&gameState);
				break;
			case 'e':
				edit_game(&gameState);
				clear_generations(&gameState);
				break;
			case 'c':
				set_world_to_value(&gameState, DEAD);
				clear_generations(&gameState);
				break;
			case 'b':
				set_world_to_value(&gameState, ALIVE);
				clear_generations(&gameState);
				break;
			case 's':
				prompt_and_save_game_to_file(&gameState, oldcur);
				break;
			case 'l':
				prompt_and_load_game_from_file(&gameState, oldcur);
				break;
			case 'q':
				running = FALSE;
				break;
			
			default: break;
		}
	}

	endwin();
	return 0;
}