#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXMENS 1024

// Variables

const int deck_size = 50;

int cards[50];

// Funciones

int how_many_cards(void) {
    int cards_quantity;
    FILE *cards_file;
    char aux[500];

    if ((cards_file = fopen("cards.txt", "r")) == NULL) {
	    printf("Error: no se encontró el archivo de tarjetas (cards.txt)\n");
	    exit(1);
    }

    cards_quantity = 0;

    while (!feof(cards_file)) {
	    fgets(aux, 500, cards_file);
	    cards_quantity++;
    }

    fclose(cards_file);

    return (cards_quantity);
}

void shuffle_cards_deck(void) {

    int i, j;

    srandom(time(0));

    for (i = 0; i <= deck_size - 1; i++) {
	    // Genero un numero entre 1 y la cantidad de tarjetas
        int random;
        random = (rand() % (deck_size)) + 1;

        // por las dudas me fijo que no este repetido
        j = i;

        while (j >= 0) {
            j--;
            if (cards[j] == random) {
                j = i;
                random++;
                if (random > deck_size) {
                    random = 1;
                }
            }
        }

        cards[i] = random;
    }

}

void get_card(int index, char *card) {
    int i;
    int id;
    FILE *cards_file;

    id = cards[index - 1];

    if ((cards_file = fopen("cards.txt", "r")) == NULL) {
	    printf("Error: no se encontró el archivo de tarjetas (cards.txt)\n");
	    exit(1);
    }

    for (i = 1; i <= id; i++) {

        fgets(card, MAXMENS, cards_file);

    }

    card[strlen(card) - 1] = '\0';

    fclose(cards_file);
}
