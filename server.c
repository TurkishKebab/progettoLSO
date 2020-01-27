#include "boardUtility.h"
#include "parser.h"
#include <netinet/in.h> //conversioni
#include <netinet/ip.h> //struttura
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define MAX_BUF 100

char *users;

int tryLogin(int clientDescriptor);
void disconnettiClient();
int registraClient(int);
void timer(void *args);
void *gestisci(void *descriptor);
char grigliaDiGiocoConPacchiSenzaOstacoli[ROWS][COLUMNS];
char grigliaOstacoliSenzaPacchi[ROWS][COLUMNS];
int numeroClient = 0;
time_t timerCount = TIME_LIMIT_IN_SECONDS;

int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Wrong parameters number(Usage: ./server usersFile)\n");
    exit(-1);
  }

  users = argv[1];
  int socketDesc, clientDesc;
  int *thread_desc;
  pthread_t tid;
  struct sockaddr_in mio_indirizzo;
  mio_indirizzo.sin_family = AF_INET;
  mio_indirizzo.sin_port = htons(5200);
  mio_indirizzo.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((socketDesc = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    perror("Impossibile creare socket"), exit(-1);
  if ((bind(socketDesc, (struct sockaddr *)&mio_indirizzo,
            sizeof(mio_indirizzo))) < 0)
    perror("Impossibile effettuare bind"), exit(-1);

  inizializzaGrigliaVuota(grigliaDiGiocoConPacchiSenzaOstacoli);
  riempiGrigliaConPacchiInPosizioniGenerateCasualmente(
      grigliaDiGiocoConPacchiSenzaOstacoli);
  generaPosizioneOstacoli(grigliaDiGiocoConPacchiSenzaOstacoli,
                          grigliaOstacoliSenzaPacchi);
  while (1 == 1) {
    if (listen(socketDesc, 10) < 0)
      perror("Impossibile mettersi in ascolto"), exit(-1);
    printf("In ascolto..\n");
    if ((clientDesc = accept(socketDesc, NULL, NULL)) < 0) {
      perror("Impossibile effettuare connessione\n");
      exit(-1);
    }
    printf("%d", clientDesc);
    numeroClient++;
    printf("Connessione effettuata (totale client connessi: %d)\n",
           numeroClient);
    // creo un puntatore per il socket del client e lo passo al thread
    thread_desc = (int *)malloc(sizeof(int));
    *thread_desc = clientDesc;
    pthread_create(&tid, NULL, gestisci, (void *)thread_desc);
  }
  close(clientDesc);
  close(socketDesc);
  return 0;
}

int tryLogin(int clientDesc) {
  // TODO proteggere con un mutex
  char *userName = (char *)calloc(MAX_BUF, 1);
  char *password = (char *)calloc(MAX_BUF, 1);
  int dimName, dimPwd;
  read(clientDesc, &dimName, sizeof(int));
  read(clientDesc, &dimPwd, sizeof(int));
  read(clientDesc, userName, dimName);
  read(clientDesc, password, dimPwd);

  int ret = 0;
  if (validateLogin(userName, password, users))
    ret = 1;

  return ret;
}

void *gestisci(void *descriptor) {
  int bufferSend[2] = {0};
  int bufferReceive[2] = {1};
  int client_sd;
  int ret = 1;
  client_sd = *(int *)descriptor;

  printf("server: gestisci sd = %d \n", client_sd);

  while (1) {
    read(client_sd, bufferReceive, sizeof(bufferReceive));

    if (bufferReceive[0] == 2) {
      int ret = registraClient(client_sd);
      if (!ret) {
        write(client_sd, &ret, sizeof(ret));
        printf("Impossibile registrare utente, riprovare\n");
      } else {
        int ret = 1;
        write(client_sd, &ret, sizeof(ret));
        printf("Utente registrato con successo\n");
      }
    }

    else if (bufferReceive[0] == 1) {
      int grantAccess = tryLogin(client_sd);
      if (grantAccess) {
        write(client_sd, &grantAccess, sizeof(grantAccess));
        inserisciPlayerNellaGrigliaInPosizioneCasuale(
            grigliaDiGiocoConPacchiSenzaOstacoli, grigliaOstacoliSenzaPacchi);

        write(client_sd, grigliaDiGiocoConPacchiSenzaOstacoli,
              sizeof(grigliaDiGiocoConPacchiSenzaOstacoli));
        /* while (1) {
          sleep(1);
          timer--;
          printf("%ld\n", timer);
          if (timer == 0) {
            inizializzaGrigliaVuota(grigliaDiGiocoConPacchiSenzaOstacoli);
           riempiGrigliaConPacchiInPosizioniGenerateCasualmente(
                grigliaDiGiocoConPacchiSenzaOstacoli);
            generaPosizioneOstacoli(grigliaDiGiocoConPacchiSenzaOstacoli,
                                 grigliaOstacoliSenzaPacchi);
           inserisciPlayerNellaGrigliaInPosizioneCasuale(
              grigliaDiGiocoConPacchiSenzaOstacoli, grigliaOstacoliSenzaPacchi);
           write(client_sd, grigliaDiGiocoConPacchiSenzaOstacoli,
                sizeof(grigliaDiGiocoConPacchiSenzaOstacoli));
           timer = TIME_LIMIT_IN_SECONDS;
         }
       }*/
        // userMovement();
      } else {
        write(client_sd, &grantAccess, sizeof(grantAccess));
      }
    }

    else if (bufferReceive[0] == 3) {
      disconnettiClient(client_sd, descriptor);
      break;
    }

    else {
      printf("Input invalido, uscita...\n");
      close(client_sd);
      free(descriptor);
      break;
    }
  }
  printf("uscita thread\n");
  pthread_exit(NULL);
}

void disconnettiClient(int clientDescriptor, int *threadDescriptor) {
  numeroClient--;
  printf("Client disconnesso (client attuali: %d)\n", numeroClient);
  close(clientDescriptor);
  free(threadDescriptor);
}

int registraClient(int clientDesc) {
  char *userName = (char *)calloc(MAX_BUF, 1);
  char *password = (char *)calloc(MAX_BUF, 1);
  int dimName, dimPwd;
  read(clientDesc, &dimName, sizeof(int));
  read(clientDesc, &dimPwd, sizeof(int));
  read(clientDesc, userName, dimName);
  read(clientDesc, password, dimPwd);
  // printf("%s:%d\n%s:%d\n", userName, dimName, password, dimPwd);
  // TODO proteggere con un mutex
  int ret = appendPlayer(userName, password, users);

  return ret;
}

void timer(void *args) {

  // TODO
}