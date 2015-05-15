#include "game_of_life.h"
#include <math.h>

inline char get_cell_at_pos(game_state* state, int line, int col){
	int result = *(state->current_world + line * state->cols + col);
	return result;
}

inline void set_cell_at_pos(game_state* state, int line, int col, char value){
	*(state->current_world + line * state->cols + col) = value;
}

void set_world_to_value(game_state* state, char value){
	for(int line = 0; line < state->lines; line++){
		for(int col = 0; col < state->cols; col++){
			set_cell_at_pos(state, line, col, value);
		}
	}
}

inline void toggle_cell_at_pos(game_state* state, int line, int col){
	char current_value = get_cell_at_pos(state, line, col);
	set_cell_at_pos(state, line, col, !current_value);
}

inline void set_next_state(game_state* state, int line, int col, char value){
	*(state->next_world + line * state->cols + col) = value;
}

int count_alive_neighbors(game_state* state, int line, int col){
	int result = 0;

	//TODO: This function is called a lot. Make it fast!
	for(int current_line = line - 1; current_line < line + 2; current_line++){
		if(current_line < 0 || current_line >= state->lines)
			continue;
		for(int current_col = col - 1; current_col < col + 2; current_col++){
			if(current_col < 0 || current_col >= state->cols)
				continue;
			if(current_line == line && current_col == col)
				continue;

			if(is_cell_alive(state, current_line, current_col))
				result++;

		}
	}

	return result;
}

// Tries to initialize the game state.
// Returns 0 if the memory allocation fails, returns non-zero otherwise.
int init_game_state(game_state* state, int cols, int lines){
	state->current_world = (char*) malloc(2 *lines * cols * 
										  sizeof(*state->current_world));
	if(!state->current_world)
		return 0;

	state->next_world = state->current_world + (lines * cols);
	state->cols = cols;
	state->lines = lines;
	clear_generations(state);

	return 1;
}

inline void clear_generations(game_state* state){
	state->generations = 0;
}

void randomize_game(game_state* state){
	for(int line = 0; line < state->lines; line++){
		for(int col = 0; col < state->cols; col++){
			int random = rand();
			if(random < RAND_MAX / 2){
				set_next_state(state, line, col, DEAD);
			}
			else
				set_next_state(state, line, col, ALIVE);
		}
	}
	//TODO: create a swap function / macro
	char* tmp = state->current_world;
	state->current_world = state->next_world;
	state->next_world = tmp;
}

void update_game_state(game_state* state){
	for(int col = 0; col < state->cols; col++){
		for(int line = 0; line < state->lines; line++){
			int is_current_cell_alive = is_cell_alive(state, line, col);
			int current_cell_alive_neighbors = count_alive_neighbors(state, line, col);

			int current_cell_next_state = is_current_cell_alive; //TODO: reuse the is_current_cell_alive variable

			if(is_current_cell_alive){
				if(current_cell_alive_neighbors < 2 || current_cell_alive_neighbors > 3){
					current_cell_next_state = DEAD;
				}
			}
			else {
				if(current_cell_alive_neighbors == 3){
					current_cell_next_state = ALIVE;
				}
			}

			set_next_state(state, line, col, current_cell_next_state);
		}
	}

	char* tmp = state->current_world;
	state->current_world = state->next_world;
	state->next_world = tmp;
	state->generations++;
}

inline int is_cell_alive(game_state* state, int line, int col){
	int result = get_cell_at_pos(state, line, col);
	return result;
}