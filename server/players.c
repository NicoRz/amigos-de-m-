#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    char name[9];
    int ready;
    int current_card;
    int votes;
    int cards_obtained;
} playersStruct;

playersStruct players[1024];

int players_quantity;

// Funciones

void init_players(void) {
    int id;
    players_quantity = 0;

    for (id = 1; id < FD_SETSIZE; id++) {
    	players[id].name[0] = '\0';
    	players[id].votes = 0;
    	players[id].current_card = 0;
        players[id].cards_obtained = 0;
    	players[id].ready = 0;
    }
}

void add_player(int id, char *name) {
    players_quantity++;
    snprintf(players[id].name, 9, "%s", name);
    players[id].current_card = 1;
    players[id].ready = 0;
    players[id].votes = 0;
    players[id].cards_obtained = 0;
}

void delete_player(int id) {
    players_quantity--;
    players[id].name[0] = '\0';
    players[id].current_card = 0;
    players[id].ready = 0;
    players[id].votes = 0;
    players[id].cards_obtained = 0;
}

void player_ready(int id) {
    players[id].ready = 1;
}

void player_leave(int id) {
    players[id].ready = 0;
}

int how_many_players(void) {
    return (players_quantity);
}

void see_player(int id, char *name) {
    snprintf(name, 9, "%s", players[id].name);
}

void get_name(int id, char *name) {
    snprintf(name, 9, "%s", players[id].name);
}

int get_ready(int id) {
    return (players[id].ready);
}

int is_player(int id) {
    return (players[id].name[0] != '\0');
}

int player_copy(char *name) {
    int copy;
    int id;

    id = 1;
    copy = 0;

    while ((!copy) && (id < FD_SETSIZE)) {

        if (players[id].current_card != 0) {
            if (strcmp(name, players[id].name) == 0) {
                copy = 1;
            }
        }

        id++;
    }

    return (copy);
}

void add_vote(int id) {
    if (id > 0) {
        players[id].votes++;
    }
}

void reset_votes(void) {
    int id;
    id = 1;

    while (id < FD_SETSIZE) {
        players[id].votes = 0;
        id++;
    }
}

void add_card(int id) {
    players[id].cards_obtained++;
}

int get_most_voted(void) {
    int id;
    int most_voted;
    int votes;

    id = 1;
    votes = 0;

    while (id < FD_SETSIZE) {
        if (players[id].votes > votes) {
            votes = players[id].votes;
            most_voted = id;
        } else {
            if (players[id].votes == votes) {
                most_voted = -1;
            }
        }

        id++;
    }

    return (most_voted);
}

int get_most_cards(void) {
    int id;
    int most_cards;
    int cards;

    id = 1;
    cards = 0;

    while (id < FD_SETSIZE) {
        if (players[id].cards_obtained > cards) {
            cards = players[id].cards_obtained;
            most_cards = id;
        }

        id++;
    }

    return (most_cards);
}

int get_loser(void) {
    int loser;
    int id;

    loser = -1;
    id = 1;

    while (id < FD_SETSIZE) {
        if (players[id].cards_obtained == 5) {
            loser = id;
        }

        id++;
    }

    return (loser);
}
