#define _XOPEN_SOURCE_EXTENDED 1

#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <locale.h>
#include <wchar.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "game_of_life.c"

//TODO: Implement getstr so the directional keys can be used (See TODO below)
//TODO: Create a default save directory and search the files in there while loading.
//      The user should be able to choose the file using the up/down keys
//TODO: Handle change of terminal size events
//TODO: Use stdint
//TODO: The boundaries of the world are creating some forms that shouldn't exist. 
//      (e.g. when a glider reaches the end of the world). This should be handed more
//      clearer.
//README: Should the genarations be saved to the file?

#define GAME_DELAY ((__useconds_t)100000)
#define UPPER_HALF_BLOCK 0x2580
#define LOWER_HALF_BLOCK 0x2584
#define FULL_BLOCK 0x2588

#define COLOR_CELL 1
#define COLOR_EDITING_CELL_ALIVE_OTHER_CELL_DEAD 2
#define COLOR_EDITING_CELL_ALIVE_OTHER_CELL_ALIVE 3
#define COLOR_EDITING_CELL_DEAD_OTHER_CELL_DEAD 4
#define COLOR_EDITING_CELL_DEAD_OTHER_CELL_ALIVE 5

#define COLOR_NORMAL_TEXT COLOR_CELL
#define COLOR_WARNING_TEXT 6
#define COLOR_QUESTION_TEXT 7
#define COLOR_INVERTED_TEXT 8

int init_curses(WINDOW** main_win, int* oldcur){
	*main_win = initscr();
	start_color();

	init_pair(COLOR_CELL, COLOR_WHITE, COLOR_BLACK);
	//README:The number of colors could probably be reduced by inverting the upper 
	//       and lower blocks, but it would be confusing.
	init_pair(COLOR_EDITING_CELL_ALIVE_OTHER_CELL_DEAD, COLOR_BLUE, COLOR_BLACK);
	init_pair(COLOR_EDITING_CELL_ALIVE_OTHER_CELL_ALIVE, COLOR_BLUE, COLOR_WHITE);
	init_pair(COLOR_EDITING_CELL_DEAD_OTHER_CELL_DEAD, COLOR_RED, COLOR_BLACK);
	init_pair(COLOR_EDITING_CELL_DEAD_OTHER_CELL_ALIVE, COLOR_RED, COLOR_WHITE);

	init_pair(COLOR_WARNING_TEXT, COLOR_WHITE, COLOR_BLUE);
	init_pair(COLOR_QUESTION_TEXT, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_INVERTED_TEXT, COLOR_BLUE, COLOR_WHITE);

	*oldcur = curs_set(0);
	noecho();
	cbreak();	

	return 1;
}

void misc_inits(){
	srand(time(NULL));
}

void discard_input_buffer(){
	int c;
	while((c = getch()) != '\n' && c != EOF);
}

char wait_for_keypress() {
	timeout(-1);
	char result = getch();
	
	timeout(0);	
	discard_input_buffer();

	return result;
}

void put_info_with_color(int color, char* text){
	move(LINES -1, 0);
	attron(COLOR_PAIR(color));
	addnstr(text, COLS);
}

void vprint_info_with_color(int color, char* text, va_list args){
	char* text_buffer = (char*) alloca(COLS * sizeof(char) + 1);
	vsnprintf(text_buffer, COLS + 1, text, args);

	put_info_with_color(color, text_buffer);
}

void print_info(char* text, ...){
	va_list args;
	va_start(args, text);
	vprint_info_with_color(COLOR_INVERTED_TEXT, text, args);
	va_end(args);	
}

void print_info_and_block(char* text, ...) {
	va_list args;
	va_start(args, text);

	char* continue_msg = "(Press any key to continue) ";
	char* msg = (char*) alloca(strlen(text) + strlen(continue_msg) + 1);
	strcpy(msg, text);
	strcat(msg, continue_msg);

	vprint_info_with_color(COLOR_WARNING_TEXT, msg, args);
	wait_for_keypress();
	va_end(args);
}

void clear_display_area(){
	attron(COLOR_PAIR(1));
	for(int col = 0; col < COLS; col++){
		move(LINES - 1, col);
		addch(' ');
	}
}

void validate_and_apply(game_state* state, int* col, int* line,
						int new_col, int new_line){

	if(new_col >= 0 && new_col < state->cols)
		*col = new_col;
	if(new_line >= 0 && new_line < state->lines)
		*line = new_line;

}

void draw_game_without_refresh(game_state* state, int show_generations){
	//Each line in the terminal represents two lines of the game
	//(using upper and lower half block characters)
	for(int line = 0; line < state->lines; line += 2){
		for(int col = 0; col < state->cols; col++){
			cchar_t temp;
			temp.attr = COLOR_PAIR(COLOR_CELL);

			int is_upper_cell_alive = is_cell_alive(state, line, col);
			int is_lower_cell_alive = is_cell_alive(state, line + 1, col);
			if(is_upper_cell_alive && is_lower_cell_alive)
				temp.chars[0] = FULL_BLOCK;
			else if(is_upper_cell_alive)
				temp.chars[0] = UPPER_HALF_BLOCK;
			else if(is_lower_cell_alive)
				temp.chars[0] = LOWER_HALF_BLOCK;
			else
				temp.chars[0] = ' ';
			
			temp.chars[1] = 0; // Null-terminating

			mvadd_wch(line / 2, col, &temp); 
		}
	}

	if(show_generations){
		clear_display_area();
		print_info("Generation: %d ", state->generations);
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

	cchar_t temp;

	int is_editing_cell_upper_cell = !(line % 2);
	int other_line = is_editing_cell_upper_cell ? line + 1 : line - 1;

	int is_editing_cell_alive = is_cell_alive(state, line, col);
	int is_other_cell_alive = is_cell_alive(state, other_line, col);

	if(is_editing_cell_alive && is_other_cell_alive)
		temp.attr = COLOR_PAIR(COLOR_EDITING_CELL_ALIVE_OTHER_CELL_ALIVE);
	else if(is_editing_cell_alive && !is_other_cell_alive)
		temp.attr = COLOR_PAIR(COLOR_EDITING_CELL_ALIVE_OTHER_CELL_DEAD);
	else if(!is_editing_cell_alive && is_other_cell_alive)
		temp.attr = COLOR_PAIR(COLOR_EDITING_CELL_DEAD_OTHER_CELL_ALIVE);
	else
		temp.attr = COLOR_PAIR(COLOR_EDITING_CELL_DEAD_OTHER_CELL_DEAD);

	if(is_editing_cell_upper_cell)
		temp.chars[0] = UPPER_HALF_BLOCK;
	else
		temp.chars[0] = LOWER_HALF_BLOCK;
	
	temp.chars[1] = 0; // Null-terminating

	mvadd_wch(line / 2, col, &temp); 
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
	print_info("  Edit Mode  ");

	timeout(-1);

	int col = 0;
	int line = 0;

	while(TRUE){
		draw_game_edit_mode(state, line, col);
		
		int input_char = getch();

		switch(input_char){
			case 'q':
				return;
			case 'a':
				set_cell_at_pos(state, line, col, ALIVE);
				break;
			case 'd':
				set_cell_at_pos(state, line, col, DEAD);
				break;
			case 't':
				toggle_cell_at_pos(state, line, col);
				break;
			case 27: //Escape char
				input_char = getch();
				if(input_char != 91) //Expected char
					break;

				input_char = getch();
				int new_col = col;
				int new_line = line;

				switch (input_char){
					case 68: //Left
						new_col--;
						break;
					case 65: //Up
						new_line--;
						break;
					case 67: //Right
						new_col++;
						break;
					case 66: //Down
						new_line++;
						break;
					case 72: //Home
						new_col = 0;
						break;
					case 70: //End
						new_col = state->cols - 1;
						break;
					case 53: //may be PgUp
						if(getch() != 126) break;
						new_line = 0;

						break;
					case 54: //may be PgDown
						if(getch() != 126) break;
						new_line = state->lines - 1;

						break;
					default:	
						break;
				}
				validate_and_apply(state, &col, &line, new_col, new_line);
				break;
			default:
				break;
		}
	}
}

int prompt_yes_no(char* msg) {
	char* question_msg = ":[y/N] ";
	char* msg_buffer = (char*) alloca(strlen(msg) + strlen(question_msg) + 1);

	strcpy(msg_buffer, msg);
	strcat(msg_buffer, question_msg);

	put_info_with_color(COLOR_QUESTION_TEXT, msg_buffer);

	switch (wait_for_keypress()){
		case 'y':
		case 'Y':
			return TRUE;
		default:
			return FALSE;
	}
}

void prompt_filename(char* filename, int oldcur){
	echo();
	curs_set(oldcur);

	char* msg = "File name:";
	clear_display_area();
	print_info(msg);

	move(LINES - 1, strlen(msg) + 1);
	attron(COLOR_PAIR(COLOR_NORMAL_TEXT));
	getstr(filename);

	noecho();
	curs_set(0);
}

int save_game_to_file(game_state* state, char* filename){
	FILE* output_file = fopen(filename, "wb");

	if(!output_file) return FALSE;

	size_t cell_size = sizeof(*state->current_world);

	fwrite(&(state->lines), sizeof(state->lines), 1, output_file);
	fwrite(&(state->cols), sizeof(state->cols), 1, output_file);

	int n_cells = state->lines * state->cols;
	fwrite(state->current_world, cell_size, n_cells, output_file);

	fclose(output_file);
	return TRUE;
}

int load_game_from_file(game_state* state, char* filename){
	FILE* input_file = fopen(filename, "rb");

	if(!input_file) return FALSE;

	size_t cell_size = sizeof(*state->current_world);
	int lines_in_file;
	int cols_in_file;

	fread(&lines_in_file, sizeof(lines_in_file), 1, input_file);
	fread(&cols_in_file, sizeof(cols_in_file), 1, input_file);

	if(lines_in_file > state->lines){
		if(!prompt_yes_no("Too many lines. Would you like to clip it? "))
			return FALSE;
		
		lines_in_file = state->lines;
	}

	int cells_to_ignore = 0;
	if(cols_in_file > state->cols){
		if(!prompt_yes_no("Too many columns. Would you like to clip it? "))
			return FALSE;

		cells_to_ignore = cols_in_file - state->cols;
		cols_in_file = state->cols;
	}

	int padding_line = 0;
	int padding_col = 0;
	if(lines_in_file < state->lines || cols_in_file < state->cols){
		set_world_to_value(state, DEAD);
		if(prompt_yes_no("World too small. Would you like to center it? ")){
			if(lines_in_file < state->lines)
				padding_line = (state->lines - lines_in_file) / 2;

			if(cols_in_file < state->cols)
				padding_col = (state->cols - cols_in_file) / 2;
		}
	}

	char* cells_ptr = state->current_world + 
					  (padding_line * state->cols  + padding_col);

	for(int line = 0; line < lines_in_file; line++){
		fread(cells_ptr, sizeof(char), cols_in_file, input_file);

		if(cells_to_ignore){
			fseek(input_file, cells_to_ignore * cell_size, SEEK_CUR);
		}
		
		cells_ptr += state->cols;
	}

	clear_generations(state);
	fclose(input_file);

	return TRUE;
}

void prompt_and_save_game_to_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(buffer, oldcur);

	if(!save_game_to_file(state, buffer)){
		clear_display_area();
		print_info_and_block(" Couldn't save file. ");
	}
}

void prompt_and_load_game_from_file(game_state* state, int oldcur){
	char buffer[1024]; //TODO: make it big enough
	
	prompt_filename(buffer, oldcur);

	if(!load_game_from_file(state, buffer)){
		clear_display_area();
		print_info_and_block(" Couldn't open file. ");
	}
}

int main(int argc, char** argv){
	setlocale(LC_ALL, "");

	int running = TRUE;
	game_state gameState;

	WINDOW* main_win;
	int oldcur;

	if(!init_curses(&main_win, &oldcur))
		return -1;	
	
	misc_inits();

	//Can't exit without endwin. Just skip the while-loop
	if(!init_game_state(&gameState, COLS, 2* (LINES - 1)))
		running = FALSE;
	

	while(running){
		clear_display_area();
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