#include <sys/types.h>

typedef struct {
    char name[9];
    int ready;
    int votes;
    int current_card;
    int cards_obtained;
} playersStruct;

playersStruct players[1024];

int players_quantity;

// Funciones

void init_players(void);

void add_player(int id, char *name);

void delete_player(int id);

void player_ready(int id);

void player_leave(int id);

int how_many_players(void);

void see_player(int id, char *name);

void get_name(int id, char *name);

int get_ready(int id);

int is_player(int id);

int player_copy(char *name);

void add_vote(int id);

void reset_votes(void);

void add_card(int id);

int get_most_voted(void);

int get_loser(void);

int get_most_cards(void);
