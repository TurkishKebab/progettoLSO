#include "boardUtility.h"
#include "list.h"
#include "parser.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h> //conversioni
#include <netinet/in.h>
#include <netinet/ip.h> //struttura
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int getTimer();
void rimuoviVecchioPlayer();
int cambiatoRound(int posizione[2]);
void printTimer();
void play();
int tryLogin();
void printMenu();
int connettiAlServer(char **argv);
char *ipResolver(char **argv);
int registrati();
int gestisci();
char getUserInput();
void clientCrashHandler();
void serverCrashHandler();
int serverCaduto();
void esciDalServer();
char getInput();
int isCorrect(char);
/*/////////////////////////////*/
int socketDesc;
char grigliaDiGioco[ROWS][COLUMNS];
/*//////////////////////////////*/
int main(int argc, char **argv) {
  signal(SIGINT, clientCrashHandler); /* CTRL-C */
  signal(SIGHUP, clientCrashHandler); /* Chiusura della console */
  signal(SIGQUIT, clientCrashHandler);
  signal(SIGTSTP, clientCrashHandler); /* CTRL-Z*/
  signal(SIGTERM, clientCrashHandler); /* generato da 'kill' */
  signal(SIGPIPE, serverCrashHandler);

  char bufferReceive[2];
  if (argc != 3)
    perror("Inserire indirizzo ip/url e porta (./client 127.0.0.1 5200)"),
        exit(-1);

  if ((socketDesc = connettiAlServer(argv)) < 0)
    exit(-1);

  gestisci(socketDesc);
  close(socketDesc);
  exit(0);
}
void esciDalServer() {
  int msg = 3;
  printf("Uscita in corso\n");
  write(socketDesc, &msg, sizeof(int));
  close(socketDesc);
}
int connettiAlServer(char **argv) {
  char *indirizzoServer;
  uint16_t porta = strtoul(argv[2], NULL, 10);
  indirizzoServer = ipResolver(argv);
  struct sockaddr_in mio_indirizzo;
  mio_indirizzo.sin_family = AF_INET;
  mio_indirizzo.sin_port = htons(porta);
  inet_aton(indirizzoServer, &mio_indirizzo.sin_addr);

  if ((socketDesc = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    perror("Impossibile creare socket"), exit(-1);

  else
    printf("Socket creato\n");

  if (connect(socketDesc, (struct sockaddr *)&mio_indirizzo,
              sizeof(mio_indirizzo)) < 0)
    perror("Impossibile connettersi"), exit(-1);

  else
    printf("Connesso a %s\n", indirizzoServer);
  return socketDesc;
}

int gestisci() {
  char choice;
  int msg;
  while (1) {
    printMenu();
    scanf("%c", &choice);
    fflush(stdin);
    system("clear");
    if (choice == '3') {
      esciDalServer();
      return (0);
    } else if (choice == '2') {
      registrati();
    } else if (choice == '1') {
      if (tryLogin()) {
        play();
      }
    } else {
      printf("Wrong input\n");
      // sleep(1);
    }
  }
}

char getInput() {
  char input;
  int done = 0;
  printf("Inserisci comando: ");
  while (!done) {
    scanf("%c", &input);
    fflush(stdin);
    if (isCorrect(input))
      done = 1;
  }
  return input;
}

int isCorrect(char input) {
  switch (input) {
  case 'w': // muovi avanti
  case 'W':
  case 'a': // muovi sinistra
  case 'A':
  case 's': // muovi sotto
  case 'S':
  case 'd': // muovi destra
  case 'D':
  case 'e': // esci
  case 'E':
  case 't': // stampa timer
  case 'T':
  case 'P': // prendi pacco
  case 'p':
  case 'c': // consegna
  case 'C':
    return 1;
    break;
  default:
    return 0;
    break;
  }
}
int serverCaduto() {
  char msg = 'y';
  if (read(socketDesc, &msg, sizeof(char)) == 0)
    return 1;
  else {
    msg = 'y';
    write(socketDesc, &msg, sizeof(msg));
  }
  return 0;
}
void play() {
  PlayerStats giocatore = NULL;
  int score, deploy[2], position[2], timer;
  int exitFlag = 0, hasApack = 0;
  while (!exitFlag) {
    if (serverCaduto())
      serverCrashHandler();

    if (read(socketDesc, grigliaDiGioco, sizeof(grigliaDiGioco)) < 1) {

      printf("Impossibile comunicare con il server\n"), exit(-1);
    }
    if (read(socketDesc, deploy, sizeof(deploy)) < 1) {
      printf("Impossibile comunicare con il server\n"), exit(-1);
    }

    if (read(socketDesc, position, sizeof(position)) < 1) {
      printf("Impossibile comunicare con il server\n"), exit(-1);
    }

    if (read(socketDesc, &score, sizeof(score)) < 1) {
      printf("Impossibile comunicare con il server\n"), exit(-1);
    }

    if (read(socketDesc, &hasApack, sizeof(hasApack)) < 1) {
      printf("Impossibile comunicare con il server\n"), exit(-1);
    }
    timer = getTimer();
    giocatore = initStats(deploy, score, position, hasApack);

    printGrid(grigliaDiGioco, giocatore);
    char send = getUserInput();
    write(socketDesc, &send, sizeof(char));
    if (send == 'e' || send == 'E') {
      printf("Disconnessione in corso...\n");
      exit(0);
    }
    if (send == 't' || send == 'T') {
      printTimer();
    }
  }
}
// TODO da finire
void rimuoviVecchioPlayer() {
  char msg = 'o';
  write(socketDesc, &msg, sizeof(char));
}
int cambiatoRound(int posizione[2]) {
  if (posizione[0] == -1 && posizione[1] == -1)
    return 1;
  return 0;
}
void printTimer() {
  int timeLeft;
  if (!serverCaduto(socketDesc)) {
    read(socketDesc, &timeLeft, sizeof(timeLeft));
    system("clear");
    fprintf(stdout, "Tempo restante: %d...\n", timeLeft);
    sleep(1);
  }
}
int getTimer() {
  int timeLeft;
  if (!serverCaduto(socketDesc)) {
    read(socketDesc, &timeLeft, sizeof(timeLeft));
  }
  return timeLeft;
}
int tryLogin() {
  int msg = 1;
  write(socketDesc, &msg, sizeof(int));
  system("clear");
  printf("Inserisci i dati per il Login\n");

  char username[20];
  char password[20];

  printf("Inserisci nome utente(MAX 20 caratteri): ");
  scanf("%s", username);
  printf("\nInserisci password(MAX 20 caratteri):");
  scanf("%s", password);

  int dimUname = strlen(username), dimPwd = strlen(password);

  if (write(socketDesc, &dimUname, sizeof(dimUname)) < 0)
    return 0;

  if (write(socketDesc, &dimPwd, sizeof(dimPwd)) < 0)
    return 0;

  if (write(socketDesc, username, dimUname) < 0)
    return 0;

  if (write(socketDesc, password, dimPwd) < 0)
    return 0;

  char validate;
  int ret;
  read(socketDesc, &validate, 1);
  if (validate == 'y') {
    ret = 1;
    printf("Accesso effettuato\n");
    sleep(1);

  } else if (validate == 'n') {
    printf("Credenziali Errate o Login già effettuato\n");
    ret = 0;
    sleep(1);
  }

  return ret;
}

// TODO da modificare/cancellare
char getUserInput() {
  fflush(stdin);
  char c;
  c = getchar();
  int daIgnorare;
  while ((daIgnorare = getchar()) != '\n' && daIgnorare != EOF) {
  }
  return c;
}

int registrati() {
  int msg = 2;
  write(socketDesc, &msg, sizeof(int));
  char username[20];
  char password[20];
  system("clear");
  printf("Inserisci nome utente(MAX 20 caratteri): ");
  scanf("%s", username);
  printf("\nInserisci password(MAX 20 caratteri):");
  scanf("%s", password);
  int dimUname = strlen(username), dimPwd = strlen(password);
  if (write(socketDesc, &dimUname, sizeof(dimUname)) < 0)
    return 0;
  if (write(socketDesc, &dimPwd, sizeof(dimPwd)) < 0)
    return 0;
  if (write(socketDesc, username, dimUname) < 0)
    return 0;
  if (write(socketDesc, password, dimPwd) < 0)
    return 0;

  char validate;
  int ret;
  read(socketDesc, &validate, sizeof(char));
  if (validate == 'y') {
    ret = 1;
    printf("Registrato con successo\n");
    sleep(1);
  }
  if (validate == 'n') {
    ret = 0;
    printf("Registrazione fallita\n");
    sleep(1);
  }

  return ret;
}

char *ipResolver(char **argv) {
  char *ipAddress;
  struct hostent *hp;
  hp = gethostbyname(argv[1]);
  if (!hp) {
    perror("Impossibile risolvere l'indirizzo ip\n");
    sleep(1);
    exit(-1);
  }
  printf("Address:\t%s\n", inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));
  return inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]);
}

void clientCrashHandler() {
  int msg = 3;
  int rec = 0;
  printf("\nChiusura client...\n");
  do {
    write(socketDesc, &msg, sizeof(int));
    read(socketDesc, &rec, sizeof(int));
  } while (rec == 0);
  close(socketDesc);
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  exit(0);
}
void serverCrashHandler() {
  system("clear");
  printf("Il server è stato spento o è irraggiungibile\n");
  close(socketDesc);
  signal(SIGPIPE, SIG_IGN);
  premiEnterPerContinuare();
  exit(0);
}