#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>
#include <errno.h>
#include "players.h"
#include "cards.h"
#include "../protocol/protocol.h"

void endgame(int sig);

void get_id(char *message_receive, char *message_id);

void set_connection(char *message_receive);

void show_players(void);

void announce_game(int playerSocketID);

void announce_ready(int playerSocketID);

void announce_leave(int playerSocketID);

void send_card(int player_id, int card_number);

void evaluate_end_of_game(int loser, int current_card);

int socketID;
int playerSocketID;
fd_set set;
fd_set copiaset;
extern int errno;

int main(int argc, char *argv[]) {
	int n;
    int pid;
    int port = 2222;
    int start;
    int order;
    int resp;
    int game;
    int player_votes;
    int loser;
    int ready_players = 0;
    int current_card;
    socklen_t clilen;
    char message_id[5];
    char name[9];
    char message_sent[MAXMENS];
    char message_receive[MAXMENS];
    struct sockaddr_in cli;
    struct sockaddr_in server;

    signal(SIGCHLD, endgame);

    if (argc == 2) {

		port = atoi(argv[1]);

		if (port == 0) {

			printf("Ejecute %s [puerto]\n",argv[0]);

			exit (1);

		}

    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    socketID = socket(PF_INET, SOCK_STREAM, 0);

    if (bind(socketID, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Error en bind");
		exit(-1);
    }

    if (listen(socketID, 3) < 0 ) {
		perror("Error en listen");
		exit(-1);
    }

    init_players();
    game = 0;

    FD_ZERO(&set);
    FD_SET(socketID, &set);

    while (1) {
    	printf("\nEsperando Jugadores:\n\n");
		start = 0;

		while (!start) {
			copiaset = set;

			if ((select(FD_SETSIZE, &copiaset, NULL, NULL, NULL) < 0) && (errno!=EINTR) ) {
	          	perror("Error en select");
				exit(-1);
		    }

		    if (FD_ISSET(socketID, &copiaset)) {
				clilen = sizeof(cli);
				playerSocketID = accept(socketID, (struct sockaddr *) &cli, &clilen);
				FD_SET(playerSocketID, &set);
		    } else {
		    	// Menu Inicio con opciones (Esperando que un jugador cree e inicie partida)

		    	for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

		    		if (FD_ISSET(playerSocketID, &copiaset)) {

		    			if ((n = read_message(playerSocketID, message_receive)) == 0) {
						    close(playerSocketID);
						    FD_CLR(playerSocketID, &set);
						} else {
							get_id(message_receive, message_id);

							if (strcmp(message_id, "CONN") == 0) {
								set_connection(message_receive);

								if (game != 0) {
								    announce_game(game);
								}
						    }

						    if (strcmp(message_id, "LIST") == 0) {
								show_players();
						    }

						    if (strcmp(message_id, "CREA") == 0) {
								player_ready(playerSocketID);

								if (game == 0) {
								    game = playerSocketID;
								    announce_game(playerSocketID);
								} 
						    }

						    if (strcmp(message_id, "JOIN") == 0) {
								player_ready(playerSocketID);
								announce_ready(playerSocketID);
						    }

						    if (strcmp(message_id, "EXIT") == 0) {
								player_leave(playerSocketID);
								announce_leave(playerSocketID);
						    }

						    if (strcmp(message_id, "STAR") == 0) {
								start = 1;
						    }
						}
		    		}
		    	}
		    }
		}

		printf("\nComienza el Juego\n");

		pid = fork();

		if (pid == 0) {
			FD_CLR(socketID, &set);
		    close(socketID);
		    shuffle_cards_deck();

		    // Cerrar conexion de jugadores que no estan listos y contar los listos para determinar la cantidad de opciones de respuesta

		    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

		    	if (FD_ISSET(playerSocketID, &set)) {
		    		
		    		if (get_ready(playerSocketID)) {
						snprintf(message_sent, MAXMENS, "STAR");
						send_message(playerSocketID, message_sent);
						ready_players++;
				    } else {
						delete_player(playerSocketID);
						FD_CLR(playerSocketID, &set);
						close(playerSocketID);
				    }
		    	}
		    }

		    loser = -1;
		    current_card = 1;

		    // Envio a los jugadores la cantidad de opciones

		    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

		    	if (FD_ISSET(playerSocketID, &set)) {
					snprintf(message_sent, MAXMENS, "%d", ready_players);
					send_message(playerSocketID, message_sent);
		    	}
			}

		    while (loser < 0) {

				// Se envia la tarjera

			    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

			    	if (FD_ISSET(playerSocketID, &set)) {
						send_card(playerSocketID, current_card);
			    	}
			    }

			    current_card++;
			    player_votes = 0;

			    // Espero las respuestas

			    while (player_votes < ready_players) {

			    	copiaset = set;

			    	if ( (select(FD_SETSIZE, &copiaset, NULL, NULL, NULL) < 0) && (errno!=EINTR) ) {
		          		perror("Error en select");
						exit(1);
			    	}

			    	for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

			    		if (FD_ISSET(playerSocketID, &copiaset)) {
			    			if ((n = read_message(playerSocketID, message_receive)) == 0) {
							    FD_CLR(playerSocketID, &set);
							    close(playerSocketID);
							} else {
								get_id(message_receive, message_id);

								if (strcmp(message_id, "VOTE") == 0) {
									char vote[MAXMENS];
									int voted_player_id;

								    memcpy(vote, message_receive + 5, 1);
								    vote[1] = '\0';

								    voted_player_id = atoi(vote);
								    add_vote(voted_player_id);
								    player_votes++;
								}
							}
			    		}
			    	}

			    	printf("\nVotos %d de %d \n", player_votes, ready_players);
			    }

			    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

			    	if (FD_ISSET(playerSocketID, &set)) {
						snprintf(message_sent, MAXMENS, "ALL DONE");
						send_message(playerSocketID, message_sent);
			    	}
			    }

			    // Evaluo las respuestas

			    int most_voted_id;
			    most_voted_id = get_most_voted();

			    if (most_voted_id > 0) {
			        add_card(most_voted_id);
			        get_name(most_voted_id, name);
			    }

			    // Anuncio ganador de tarjeta

			    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

			    	if (FD_ISSET(playerSocketID, &set)) {
			    	    if (most_voted_id > 0) {
			    	        snprintf(message_sent, MAXMENS, "WINN %s", name);
			    	    } else {
			    	        snprintf(message_sent, MAXMENS, "TIED");
			    	    }

						send_message(playerSocketID, message_sent);
			    	}
			    }

				reset_votes();

				// Evaluo fin de juego

				loser = get_loser();
				evaluate_end_of_game(loser, current_card);
		    }

            if (loser < 0) {
                int most_cards = get_most_cards();
                get_name(most_cards, name);
            } else {
                get_name(loser, name);
            }

		    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

		    	if (FD_ISSET(playerSocketID, &set)) {
					snprintf(message_sent, MAXMENS, "%s", name);
					send_message(playerSocketID, message_sent);
					close(playerSocketID);
		    	}
	    	}

		    exit(0);
		} else {
		    for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {
                if (FD_ISSET(playerSocketID, &set)) {
                    if (socketID != playerSocketID) {
                        if (get_ready(playerSocketID)) {
                            delete_player(playerSocketID);
                            FD_CLR(playerSocketID, &set);
                            close(playerSocketID);
                        }
                    }
                }
            }
		}
    }

    close(socketID);
    exit(0);
}

void endgame(int sig) {
	int status;
	wait3(&status,WNOHANG,(struct rusage *)0);
	printf("\nFinalizo el juego\n");
}

void get_id(char *message_receive, char *message_id) {
    memcpy(message_id, message_receive, 4);
    message_id[4] = '\0';
}

void set_connection(char *message_receive) {
    int i;
    char message_name[9];
    char name[9];
    char message_sent[MAXMENS];

    memcpy(message_name, message_receive + 4, 8);

    message_name[8] = '\0';

    if (!player_copy(message_name)) {

		printf("%s ingreso al Juego\n", message_name);
		add_player(playerSocketID, message_name);
		snprintf(message_sent, MAXMENS, "CONNECTION OK");
		send_message(playerSocketID, message_sent);

		for (i = 0; i < FD_SETSIZE; i++) {

		    if (FD_ISSET(i, &set)) {
				if ((i != socketID) && (i != playerSocketID)) {
				    get_name(i, name);

				    if (name[0] != '\0') {
						snprintf(message_sent, MAXMENS, "ALER %s ingreso al juego", message_name);
						send_message(i, message_sent);
				    }
				}
		    }
		}
    } else {
		snprintf(message_sent, MAXMENS, "DUPL");
		send_message(playerSocketID, message_sent);
    }
}

void show_players(void) {
    int i;
    char player_name[9];
    char message_sent[MAXMENS];

    for (i = 0; i < FD_SETSIZE; i++) {

		if (FD_ISSET(i, &set)) {

		    if (i != socketID) {
				get_name(i, player_name);

				if (player_name[0] != '\0') {
				    if (get_ready(i)) {
						snprintf(message_sent, MAXMENS, "%s esta listo para jugar", player_name);
				    } else {
						snprintf(message_sent, MAXMENS, "%s esta conectado", player_name);
				    }

				    send_message(playerSocketID, message_sent);
				}
		    }
		}
    }

    snprintf(message_sent, MAXMENS, "LIST END");
    send_message(playerSocketID, message_sent);
}

void announce_game(int playerSocketID) {
    int i;
    char player_name[9];
    char message_sent[MAXMENS];

    snprintf(message_sent, MAXMENS, "CREA OK");

    send_message(playerSocketID, message_sent);

    for (i = 0; i < FD_SETSIZE; i++) {

		if (FD_ISSET(i, &set)) {

		    if ((i != socketID) && (i != playerSocketID)) {
				get_name(playerSocketID, player_name);
				snprintf(message_sent, MAXMENS, "PLAY %s creo un Juego.", player_name);
				send_message(i, message_sent);
			}
		}
    }
}

void announce_ready(int playerSocketID) {
    int i;
    char name[9];
    char message_sent[MAXMENS];

    for (i = 0; i < FD_SETSIZE; i++) {

		if (FD_ISSET(i, &set)) {

		    if ((i != socketID) && (i != playerSocketID)) {
				get_name(playerSocketID, name);
				snprintf(message_sent, MAXMENS, "ALER %s esta listo para jugar", name);
				send_message(i, message_sent);
		    }
		}
    }
}

void announce_leave(int playerSocketID) {
    int i;
    char name[9];
    char message_sent[MAXMENS];

    for (i = 0; i < FD_SETSIZE; i++) {

		if (FD_ISSET(i, &set)) {

		    if ((i != socketID) && (i != playerSocketID)) {
				get_name(playerSocketID, name);
				snprintf(message_sent, MAXMENS, "ALER %s no desea unirse al juego", name);
				send_message(i, message_sent);
		    }
		}
    }
}

void send_card(int player_id, int card_number) {
    char message[MAXMENS];
    char card[MAXMENS];
    int i;
    char player_name[9];
    char message_sent[MAXMENS];

    get_card(card_number, card);
    snprintf(message, MAXMENS, "%s", card);
    send_message(playerSocketID, message);

    for (i = 0; i < FD_SETSIZE; i++) {

		if (FD_ISSET(i, &set)) {

		    if (i != socketID) {
				get_name(i, player_name);

				if (player_name[0] != '\0') {
					snprintf(message, MAXMENS, "%d. %s", i, player_name);
				    send_message(playerSocketID, message);
				}
		    }
		}
    }
}

void evaluate_end_of_game(int loser, int current_card) {
	char message_sent[MAXMENS];

	for (playerSocketID = 0; playerSocketID < FD_SETSIZE; playerSocketID++) {

    	if (FD_ISSET(playerSocketID, &set)) {

			if (loser < 0 && current_card < 50) {
				snprintf(message_sent, MAXMENS, "NEXT");
			} else {
				snprintf(message_sent, MAXMENS, "DONE");
			}

			send_message(playerSocketID, message_sent);
    	}
	}
}
