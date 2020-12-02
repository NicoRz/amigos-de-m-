#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "../protocol/protocol.h"

void set_name(void);

void show_menu(int state);

void show_prompt(void);

void show_players(void);

void set_options(void);

int read_alert(int state);

int create_game(void);

void join_game(void);

void start_game(void);

void wait_card(void);

void send_answer(void);

void show_result(void);

void exit_game(void);

void show_loser(void);

void wait_other_players(void);


/**
    Lista de estados del cliente:
        1. Jugador ya eligio su nombre
        2. Jugador salio del juego
        3. Jugador creo un juego
        4. Jugador se unio a un juego
        5. Jugador empezo el juego
        6. Juego finalizado
**/

int extern errno;
int socketID;
int options;

int main(int argc, char *argv[]) {
    int state;
    int option;
    int start;
    int wait;
    int port = 2222;
    char host[MAXMENS] = "localhost";
    char screen[MAXMENS];
    struct hostent *ser;
    struct sockaddr_in server;
    fd_set set;

    switch (argc) {
    	case 3:
    		port = atoi(argv[2]);
    	case 2:
    		snprintf(host,MAXMENS,"%s",argv[1]);
    		break;
    	case 1:
    		break;
    	default:
    		printf("Ejecute: %s [servidor [puerto]]\n", argv[0]);
    		exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

     if (ser = gethostbyname(host)) {
        memcpy(&server.sin_addr, ser->h_addr, ser->h_length);
     } else {
    	printf("No se encontro %s (%s)\n",host,strerror(errno));
    	printf("Ejecute: %s [servidor [puerto]]\n", argv[0]);
    	exit(1);
     }

     socketID = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

     if (socketID < 0) {
     	perror("Error:");
     	exit(1);
     }

     if (connect(socketID, (struct sockaddr *) &server, sizeof(server)) != 0) {
        printf("Servidor no disponible en %s:%d (%s)\n",host,port,strerror(errno));
        printf("Ejecute: %s [servidor [puerto]]\n", argv[0]);
        exit(1);
     }

     set_name();
     state = 1;
     show_menu(state);
     start = 0;

     while (!start) {
     	option = 0;

     	FD_ZERO(&set);
     	FD_SET(STDIN_FILENO, &set);
     	FD_SET(socketID, &set);
     	select(FD_SETSIZE, &set, NULL, NULL, NULL);

     	if (FD_ISSET(STDIN_FILENO, &set)) {
     	    fgets(screen, MAXMENS, stdin);
     	    option = atoi(screen);

     	    if (option != 1 && option != 2) {
     		    show_prompt();
     	    }
     	} else {
     	    state = read_alert(state);

     	    if (state == 5) {
     		    start = 1;
     	    }
     	}

     	switch (option) {
            case 1:
                show_players();
                show_prompt();
                break;
     	    case 2:
     	        switch (state) {
     	            case 1:
     		            if (create_game()) {
                            printf("\nEl juego se creo satisfactoriamente.\n");
                            printf("Usted decide cuando empezar el juego\n");
                            state = 3;
                            show_menu(state);
     		            }
     		            break;
     	            case 2:
                        join_game();
     		            state = 4;
                        show_menu(state);
                    break;
                    case 3:
                        start_game();
                        state = read_alert(state);

     		            if (state == 5) {
                            start = 1;
     		            }
                    break;
                    case 4:
                        exit_game();
                        state = 2;
                        show_menu(state);
                    break;
                }
            break;
     	}
     }

     // COMIENZO EL JUEGO

     wait = 5;

     while (wait != 0) {
     	printf("\tComenzando el juego en %d segundos ...\n", wait);
     	sleep(1);
     	wait--;
     }

     printf("\n----------------------------------------------------------------\n\n");
     printf("\nEmpezo el juego:\n\n");

     set_options();

     while (state == 5) {
        wait_card();
        send_answer();
        wait_other_players();
        show_result();
        state = read_alert(state);
     }

     // Fin del juego

     printf("\nJuego terminado:\n\n");
     show_loser();
     close(socketID);
     exit(0);
}

void set_name(void) {
    int res;
    int connection;
    char message[MAXMENS];
    char name[10];

    connection = 0;

    while (!connection) {
	    res = 0;

	    while (res == 0) {
	        printf("\nIngrese Nombre (8 caracteres): ");
	        fgets(name, 10, stdin);
	        name[strlen(name)-1] = '\0';
	        res = strlen(name);
	    }

	    snprintf(message, MAXMENS, "CONN");
	    strcat(message, name);
	    send_message(socketID, message);

	    res = 0;

	    while (!res) {
	        read_message(socketID, message );

	        if (strcmp(message, "CONNECTION OK") == 0) {
		        printf("\nUsted ingreso al juego\n");
		        connection = 1;
		        res = 1;
	        }

	        if (strcmp(message, "DUPL") == 0) {
                printf("\nYa existe un jugador con el nombre %s \n", name);
		        res = 1;
            }
	    }
    }
}

void show_menu(int state) {
    if (state == 1) {
	    printf("\n1-Listar Jugadores\n2-Crear Juego\n");
    }

    if (state == 2) {
	    printf("\n1-Listar Jugadores\n2-Unirse al Juego\n");
    }

    if (state == 3) {
	    printf("\n1-Listar Jugadores\n2-Empezar Juego\n");
    }

    if (state == 4) {
	    printf("\n1-Listar Jugadores\n2-Desconectarce del Juego\n");
    }

    printf("\n(1,2)=> ");

    fflush(stdout);
}

int read_alert(int state) {
    char message_receive[MAXMENS];
    char message_id[5];
    char message[MAXMENS];

    read_message(socketID, message_receive);

    memcpy(message_id, message_receive, 4);

    message_id[4] = '\0';

    memcpy(message, message_receive + 5, (strlen(message_receive) - 4));

    message[(strlen(message_receive) - 4)] = '\0';

    if (strcmp(message_id, "ALER") == 0) {
	    printf("\n\nAVISO: %s\n\n", message);
        show_prompt();
    }

    if (strcmp(message_id, "PLAY") == 0) {
        printf("\n\nAVISO: %s\n", message);
	    state = 2;
	    show_menu(state);
    }

    if (strcmp(message_id, "STAR") == 0) {
    	state = 5;
    }

    if (strcmp(message_id, "NOPL") == 0) {
	    printf("\n\nAVISO: %s\n", message);
	    state = 1;
	    show_menu(state);
    }

    if (strcmp(message_id, "NEXT") == 0) {
        state = 5;
    }

    if (strcmp(message_id, "DONE") == 0) {
        state = 6;
    }

    return (state);
}

void show_prompt(void) {
    printf("(1,2)=> ");
    fflush(stdout);
}

void show_players(void) {
    int listfin;
    char message[MAXMENS];

    printf("\nJugadores:\n");

    snprintf(message, MAXMENS, "LIST");

    send_message(socketID, message);

    listfin = 0;

    while (!listfin) {
	    read_message(socketID, message);

        if (strcmp(message, "LIST END") == 0) {
            printf("\n");
            listfin = 1;
        } else {
	        printf("\t%s\n", message);
	    }
    }
}

int create_game(void) {
    int res;
    char message[MAXMENS];
    snprintf(message, MAXMENS, "CREA");
    send_message(socketID, message);
    res = 2;
    while (res == 2) {
        read_message(socketID, message);

        if (strcmp(message, "CREA OK") == 0) {
            res = 1;
        }

        if (strcmp(message, "CREA NO") == 0) {
            res = 0;
        }

    }

    return (res);
}

void join_game(void)    {
    char message[MAXMENS];
    snprintf(message, MAXMENS, "JOIN");
    send_message(socketID, message);
}

void start_game(void) {
    char message[MAXMENS];
    snprintf(message, MAXMENS, "STAR");
    send_message(socketID, message);
}

void exit_game(void) {
    char message[MAXMENS];
    snprintf(message, MAXMENS, "EXIT");
    send_message(socketID, message);
}

void set_options(void) {
    char options_message[MAXMENS];

    read_message(socketID, options_message);
    options = atoi(options_message);
}

void wait_card(void) {
    char card[MAXMENS];
    char option[MAXMENS];
    int i = 0;

    read_message(socketID, card);

    printf("\n--------------------------TARJETA SELECCIONADA---------------------------\n\n");

    printf("\nElije a que amigo le corresponde la siguiente afirmación: \n\n ");
    printf("%s\n\n", card);

    printf("\n### OPCIONES ###\n\n");
    for (i = 0; i < options; i++) {
        read_message(socketID, option);
        printf("\t%s\n", option);
    }
}

void send_answer(void) {
    int option;
    char screen[1024];
    char answer[50];
    long start;
    fd_set set;

    option = -1;
    while (option < 0) {
        fflush(stdout);
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

	    if (select(1, &set, NULL, NULL, NULL) != 0) {
	        fgets(screen, MAXMENS, stdin);
	        option = atoi(screen);

	        if (option > 0) {
                snprintf(answer, MAXMENS, "VOTE %d", option);
                send_message(socketID, answer);
            }
        }
    }
}

void show_result(void) {
    char message[MAXMENS];
    char winner[MAXMENS];
    char message_id[5];

    read_message(socketID, message);

    memcpy(message_id, message, 4);
    message_id[4] = '\0';

    memcpy(winner, message + 5, (strlen(message) - 4));
    winner[(strlen(message) - 4)] = '\0';

    printf("\n##### RESULTADO DE LA TARJETA #####\n");

    if (strcmp(message_id, "WINN") == 0) {
        printf("\nEl jugador mas votado es: %s\n", winner);
    }

    if (strcmp(message_id, "TIED") == 0) {
        printf("\nHubo un empate, por lo que nadie recibe la tarjeta\n");
    }
}

void show_loser(void) {
    char loser[MAXMENS];
    read_message(socketID, loser);
    printf("\n\nEl amigo de m***** es: %s\n", loser);
}

void wait_other_players(void) {
    int players_done;
    char message[MAXMENS];
    struct timeval t;
    fd_set set;

    printf("\nEsperando al resto...\n");

    players_done = 0;

    while (!players_done) {
        
        message[0] = '\0';

        FD_ZERO(&set);

        FD_SET(socketID, &set);

        t.tv_sec = 300;
        t.tv_usec = 0;

        if (select(FD_SETSIZE, &set, NULL, NULL, &t) == 0) {

            printf("\nERROR en Select\n");

            close(socketID);

            exit(0);
        }

        read_message(socketID, message);

        if (strcmp(message, "ALL DONE") == 0) {

            players_done = 1;

        }
    }
}
