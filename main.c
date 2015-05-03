#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "game_of_life.c"

//TODO: Handle change of terminal size events
//TODO: Make a blocking version of display_text
//TODO: Use Unicode
//TODO: Use stdint
//TODO: Count generations
//TODO: Save game to a file
//TODO: Load game from a file
//TODO: The boundaries of the board are creating some forms that shouldn't exist. 
//      (e.g. when a glider reaches the end of the board). This should be handed more
//      clearer.

void init_curses(){
	
}

//TODO: Handle var_args.
void display_text(char* text){
	move(LINES -1, 0);
	attron(COLOR_PAIR(5));
	printw(text);
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

void draw_game_without_refresh(game_state* state){
	for(int line = 0; line < state->lines; line++){
		for(int col = 0; col < state->cols; col++){
			move(line, col);
			int is_current_cell_alive = is_cell_alive(state, line, col);
			if(is_current_cell_alive)
				attron(COLOR_PAIR(2));
			else
				attron(COLOR_PAIR(1));
			
			addch(' ');
			
		}
	}
}

void draw_game(game_state* state){
	draw_game_without_refresh(state);

	refresh();
}

void draw_game_edit_mode(game_state* state, int line, int col){
	draw_game_without_refresh(state);

	move(line, col);
	if(is_cell_alive(state, line, col))
		attron(COLOR_PAIR(3));
	else 
		attron(COLOR_PAIR(4));

	addch(' '); 

	refresh();
}

void step_game(game_state* state){
	update_game_state(state);

	draw_game(state);
}

void loop_game(game_state* state){
	timeout(0);

	while(1){
		int c = getch();
		if(c == 'q')
			return;

		update_game_state(state);
		draw_game(state);
		usleep(100000);
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

	while(1){
		draw_game_edit_mode(state, pos_y, pos_x);
		
		int input_char = getch();

		switch(input_char){
			case 'q':
				clear_text();
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
	display_text(msg);
	draw_game(state);

	move(LINES - 1, strlen(msg) + 1);
	attron(COLOR_PAIR(1));
	getstr(buffer);

	noecho();
	curs_set(0);
	clear_text();
}

int save_game_to_file(game_state* state, char* filename){
	FILE* output_file = fopen(filename, "wb");

	if(!output_file) return 0;

	fwrite(&(state->lines), sizeof(state->lines), 1, output_file);
	fwrite(&(state->cols), sizeof(state->cols), 1, output_file);

	int n_cells = state->lines * state->cols;
	fwrite(state->current_matrix, sizeof(char), n_cells, output_file);

	fclose(output_file);
	return 1;
}

int load_game_from_file(game_state* state, char* filename){
	FILE* input_file = fopen(filename, "rb");

	if(!input_file) return 0;

	int lines_in_file;
	int cols_in_file;

	fread(&lines_in_file, sizeof(lines_in_file), 1, input_file);
	fread(&cols_in_file, sizeof(cols_in_file), 1, input_file);

	//TODO: Make it user friendly.
	//File is too large. Clipping
	if(lines_in_file > state->lines){
		lines_in_file = state->lines;
	}

	if(cols_in_file > state->cols){
		cols_in_file = state->cols;
	}

	//TODO: Handle the case wherein the file is smaller than the game_state
	//      Maybe center the file world

	set_world_to_value(state, DEAD);

	char value;
	//TODO: Handle this with only one for-loop
	for(int line = 0; line < lines_in_file; line++){
		for (int col = 0; col < cols_in_file; col++){
			fread(&value, sizeof(char), 1, input_file);
			set_cell_at_pos(state, line, col, value);
		}
	}

	fclose(input_file);
}

void prompt_and_save_game_to_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(state, oldcur, buffer);

	if(!save_game_to_file(state, buffer))
		display_text(" Couldn't save file. ");//TODO: block here
}

void prompt_and_load_game_from_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(state, oldcur, buffer);

	if(load_game_from_file(state, buffer))
		display_text(" Couldn't open file. "); //TODO: block here
}

int main(int argc, char** argv){

	int running = 1;

	//TODO: Extract to an init function
	WINDOW* main_win = initscr();
	start_color();

	//TODO: Theses modes should be defined constants
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_WHITE);
	init_pair(3, COLOR_WHITE, COLOR_BLUE);
	init_pair(4, COLOR_WHITE, COLOR_RED);
	init_pair(5, COLOR_BLUE, COLOR_WHITE);

	int oldcur = curs_set(0);
	noecho();
	cbreak();
	
	game_state gameState;
	if(!init_game_state(&gameState, COLS, LINES - 1))
		return -1;

	//TODO: This belongs to the init section
	srand(time(NULL));

	while(running){
		draw_game(&gameState);

		timeout(-1);
		int c = getch();

		switch(c) {
			case 'r':
				randomize_game(&gameState);
				break;
			case ' ':
				step_game(&gameState);
				break;
			case 'p':
				loop_game(&gameState);
				break;
			case 'e':
				edit_game(&gameState);
				break;
			case 'c':
				set_world_to_value(&gameState, DEAD);
				break;
			case 'b':
				set_world_to_value(&gameState, ALIVE);
				break;
			case 's':
				prompt_and_save_game_to_file(&gameState, oldcur);
				break;
			case 'l':
				prompt_and_load_game_from_file(&gameState, oldcur);
				break;
			case 'q':
				running = 0;
				break;
			
			default: break;
		}
	}

	endwin();
	return 0;
}