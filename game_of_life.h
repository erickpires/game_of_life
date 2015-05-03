#ifndef GAME_OF_LIFE_H
#define GAME_OF_LIFE_H

#define DEAD 0
#define ALIVE 1

typedef struct {
	char* current_matrix;
	char* next_matrix;
	int cols;
	int lines;
	
} game_state;

int init_game_state(game_state*, int, int);
void randomize_game(game_state*);
void update_game_state(game_state*);
int is_cell_alive(game_state*, int, int);
void set_next_state(game_state*, int, int, char);
char get_cell_at_pos(game_state*, int, int);
void set_cell_at_pos(game_state*, int, int, char);
void set_world_to_value(game_state*, char);
void toggle_cell_at_pos(game_state*, int, int);

#endif