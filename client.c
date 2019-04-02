/* Generic */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*utility */
#include <semaphore.h>
#include <sched.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>



#define BUF_SIZE 100



//N for the number of created threads.
// client cmd args  - client [host] [portnum] [threads] [schedalg] [filename1] [filename2]
char* Host;
char* Portnum;
int numOfThreads;
char* Schedalg;//implement soon
char* Filename1;
char* Filename2;// implement soon

//for CONCUR Schedule
pthread_barrier_t barrier;


//for FIFO schedule
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int turn = 0;

// Get host information (used to establishConnection)
struct addrinfo *getHostInfo(char* host, char* port) {
  int r;
  struct addrinfo hints, *getaddrinfo_res;
  // Setup hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
    fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
    return NULL;
  }

  return getaddrinfo_res;
}

// Establish connection with host
int establishConnection(struct addrinfo *info) {
  if (info == NULL) return -1;

  int clientfd;
  for (;info != NULL; info = info->ai_next) {
    if ((clientfd = socket(info->ai_family,
                           info->ai_socktype,
                           info->ai_protocol)) < 0) {
      perror("[establishConnection:35:socket]");
      continue;
    }

    if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
      close(clientfd);
      perror("[establishConnection:42:connect]");
      continue;
    }

    freeaddrinfo(info);
    return clientfd;
  }

  freeaddrinfo(info);
  return -1;
}

// Send GET request
void GET(int clientfd, char *path) {
  char req[1000] = {0};
  sprintf(req, "GET %s HTTP/1.0\r\n\r\n", path);
  send(clientfd, req, strlen(req), 0);
}
void doThreadJobConcur(int i){
    char* buf = calloc(BUF_SIZE,1);
    while(1){

        int clientfd;
        // Establish connection with <hostname>:<port>
        clientfd = establishConnection(getHostInfo(Host, Portnum));
        if (clientfd == -1) {
            fprintf(stderr,
                    "[main:73] Failed to connect to: %s:%s%s \n",
                    Host, Portnum, Filename1);

        }

            // Send GET request > stdout
            printf("\n");
            printf("THREAD ID: %d\n", i);
            printf("***** ****** ***** *********** ****** ***** ****** ***** ****** ***** ******\n");
            //send req
            GET(clientfd, Filename1);

            //receive response
            while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
                fputs(buf, stdout);
                memset(buf, 0, BUF_SIZE);
            }
            printf("***** ****** ***** *********** ****** ***** ****** ***** ****** ***** ******\n");
            printf("\n");

            close(clientfd);
            //all threads wait for all other threads to finish before sending new requests
            pthread_barrier_wait(&barrier);


    }

}
void doThreadJobFifo(int i){
    char* buf = calloc(BUF_SIZE,1);
    while(1) {
        //is it this threads turn
        while (!(turn == i)) {
            pthread_cond_wait(&cond, &mutex);
        }
        //printf("turn = %d, thread= %d\n",turn,i);
        pthread_mutex_lock(&mutex);
        //pthread_cond_wait(&cond, &mutex);
        //printf("Its's Thread %d turn\n", i);
        int clientfd;
        // Establish connection with <hostname>:<port>
        clientfd = establishConnection(getHostInfo(Host, Portnum));
        if (clientfd == -1) {
            fprintf(stderr,
                    "[main:73] Failed to connect to: %s:%s%s \n",
                    Host, Portnum, Filename1);

        }

        // Send GET request > stdout
        printf("\n");
        printf("THREAD ID: %d\n", i);
        printf("***** ****** ***** *********** ****** ***** ****** ***** ****** ***** ******\n");
        //send req
        GET(clientfd, Filename1);
        if(turn == numOfThreads - 1){
            turn =0;
        }
        else{
            turn++;
        }
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);

        //receive response
        while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
            fputs(buf, stdout);
            memset(buf, 0, BUF_SIZE);
        }
        printf("***** ****** ***** *********** ****** ***** ****** ***** ****** ***** ******\n");
        printf("\n");

        close(clientfd);

        //wait for all threads to receive their response
        pthread_barrier_wait(&barrier);
    }
}


int main(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr, "USAGE: ./httpclient <hostname> <port> <request path>\n");
    return 1;
  }

  /* building a thread pool to bombard the shit out of My Irani's spineless server*/
  Host = argv[1];
  Portnum = argv[2];
  numOfThreads = atoi(argv[3]);
  Schedalg = argv[4];
  Filename1 = argv[5];

  pthread_t *threads = calloc(numOfThreads, sizeof(pthread_t));
  int status, i;

  //concurent scheduling?
  if(strcmp(Schedalg,"CONCUR")==0) {

      pthread_barrier_init(&barrier,NULL,numOfThreads);

      for (i = 0; i < numOfThreads; i++) {
          int *x;
          x = &i;
          status = pthread_create(&threads[i], NULL, (void *(*)(void *))doThreadJobConcur, x);
          if (status != 0) {
              printf("ERROR: MAKING THREAD POOL");
              exit(0);
          }
      }
    //FIFO scheduling?
  } else if(strcmp(Schedalg,"FIFO")==0){

      pthread_cond_init(&cond,NULL);
      pthread_mutex_init(&mutex,NULL);
      pthread_barrier_init(&barrier,NULL,numOfThreads);
      for (i = 0; i < numOfThreads; i++) {
          int *x;
          x = &i;
              status = pthread_create(&threads[i], NULL, (void *(*)(void *)) doThreadJobFifo, x);
              if (status != 0) {
                  printf("ERROR: MAKING THREAD POOL");
                  exit(0);
              }
          }
  }

    /* END BUILD THREADPOOL*/


pthread_exit(NULL);

}
