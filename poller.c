#include <stdio.h>
#include <sys/wait.h> /* signals */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h> /* gethostbyaddr */
#include <unistd.h> /* read, write, close */
#include <stdlib.h> /* exit */
#include <ctype.h>
#include <signal.h> //signal
#include <string.h>
#include <pthread.h>
#include <errno.h> //signal
#include <arpa/inet.h> //internetsockets
#include "poller.h"


static int interrupted = 0;

void main(int argc, char* argv[]) {
    int port, sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen;
    struct sockaddr* serverptr = (struct sockaddr*)&server;
    struct sockaddr* clientptr = (struct sockaddr*)&client;
    struct hostent* rem;

    static struct Party* partyList = NULL;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
    pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;
    struct ConnectionBuffer connectionBuffer;
    connectionBuffer.head = NULL;
    connectionBuffer.tail = NULL;
    connectionBuffer.count = 0;

    if (argc != 6) {
        printf("Please provide the correct input arguments.\n");
        exit(1);
    }

    port = atoi(argv[1]);
    int numThreads = atoi(argv[2]);
    int bufferSize = atoi(argv[3]);
    const char* resultsFile = argv[5];


    /*signal handlers for ctrl+C */
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror_exit("socket");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (bind(sock, serverptr, sizeof(server)) < 0) {
        perror_exit("bind");
    }

    if (listen(sock, 20) < 0) {
        perror_exit("listen");
    }

    printf("Listening for connections on port %d\n", port); // A simple message to showcase the launch of the server

    pthread_t* threads = (pthread_t*)malloc(numThreads * sizeof(pthread_t));
    if (threads == NULL) {
        perror_exit("malloc");
    }

    for (int i = 0; i < numThreads; i++) {
        struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
        if (threadArgs == NULL) {
            perror_exit("malloc");
        }
        threadArgs->filename = argv[4];
        threadArgs->partyList = &partyList;
        threadArgs->mutex = &mutex;
        threadArgs->bufferNotEmpty = &bufferNotEmpty;
        threadArgs->bufferNotFull = &bufferNotFull;
        threadArgs->connectionBuffer = &connectionBuffer;
        threadArgs->threadid = getpid() + i + 1;

        if (pthread_create(&threads[i], NULL, child_server, (void*)threadArgs) != 0) {
            perror_exit("pthread_create");
        }
    }

    int a;

    FILE* resultsFilePtr = fopen(resultsFile, "a");
    if (resultsFilePtr == NULL) {
        perror_exit("fopen");
    }

    while (1) {
        if(interrupted){
            printf("interrupted\n");
            pthread_cond_broadcast(&bufferNotEmpty);
            break;
        }

        pthread_mutex_lock(&mutex);

        while(connectionBuffer.count == bufferSize) {
            printf("buffer is full, mainthread cannot accept\n");  //printf to showcase proper buffer usage, when buffer is full
            pthread_cond_wait(&bufferNotFull,&mutex);
        }

        pthread_mutex_unlock(&mutex);

        clientlen = sizeof(client);
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0) {
            if (errno == EINTR) {
                if (interrupted) {
                    printf("interrupted\n");
                    pthread_cond_broadcast(&bufferNotEmpty); 
                    break;
                }   
            } else {
                perror_exit("accept");
            }
        }

        if (interrupted) {  // multiple interrupted checks to make sure the signal is captured
            printf("interrupted\n");
            pthread_cond_broadcast(&bufferNotEmpty);
            break;
        }

        if ((rem = gethostbyaddr((char*)&client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
            perror("gethostbyaddr");
            exit(1);
        }

        struct Connection* newConnection = (struct Connection*)malloc(sizeof(struct Connection));
        if (newConnection == NULL) {
            perror_exit("malloc");
        }
        newConnection->newsock = newsock;
        newConnection->next = NULL;

        if (connectionBuffer.head == NULL) {            //printfs used to showcase proper buffer usage
            //printf("Connections in buffer that is first\n"); 
            connectionBuffer.head = newConnection;
            connectionBuffer.tail = newConnection;
        } else {
            //printf("Connections in buffer isnt first\n");
            connectionBuffer.tail->next = newConnection;
            connectionBuffer.tail = newConnection;
        }

        connectionBuffer.count++;

        pthread_mutex_unlock(&mutex);

        pthread_cond_broadcast(&bufferNotEmpty); //This signal is broadcasted to the threads 

    }

    // Closing all remaining connections in the buffer
    struct Connection* currConnection = connectionBuffer.head;
    while (currConnection != NULL) {
        struct Connection* nextConnection = currConnection->next;
        close(currConnection->newsock);
        free(currConnection);
        currConnection = nextConnection;
    }

    fprintParties(partyList,resultsFilePtr);
    
    // cleanup
    close(newsock);
    close(sock);

    free(threads);
    fclose(resultsFilePtr);
    freeParties(partyList);

    printf("Server terminated.\n");
    exit(0);
}

void* child_server(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    const char* filename = args->filename;
    struct Party** partyList = args->partyList;
    pthread_mutex_t* mutex = args->mutex;
    pthread_cond_t* bufferNotEmpty = args->bufferNotEmpty;
    pthread_cond_t* bufferNotFull = args->bufferNotFull;
    struct ConnectionBuffer* connectionBuffer = args->connectionBuffer;
    int a = args->threadid;
    free(arg);

    char buf[1024];
    ssize_t bytes_read;
    FILE* file = fopen(filename, "a");

    if (file == NULL) {
        perror_exit("fopen");
    }

    while (1) {
        while (connectionBuffer->count == 0) {
            pthread_cond_wait(bufferNotEmpty, mutex); //waiting signal from mainthread
            if (interrupted) {
                pthread_mutex_unlock(mutex);
                fclose(file);
                return NULL;
            }
        }

        struct Connection* connection = connectionBuffer->head;
        if(connection == NULL) {
            pthread_mutex_unlock(mutex);
            printf("Connection is NULL\n");
            continue;
        }

        int newsock = connection->newsock;
        connectionBuffer->head = connection->next;
        connectionBuffer->count--;

        if(connectionBuffer->tail == connection) {          //checking if there is only one connection in the buffer
            connectionBuffer->tail = NULL;
        }

        if(connectionBuffer->head == NULL) {
            connectionBuffer->tail = NULL;                  //checking if the buffer is empty
        }

        free(connection);
        pthread_mutex_unlock(mutex);
        pthread_cond_broadcast(bufferNotFull);

        char* message = "Please send your vote:";
        write(newsock, message, strlen(message));

        bytes_read = read(newsock, buf, sizeof(buf));

        if (bytes_read > 0) {
            buf[bytes_read] = '\0';

            char name[100], lastName[100], party[100];
            sscanf(buf, "%s %s %s", name, lastName, party);

            pthread_mutex_lock(mutex);

            addParty(partyList, party, mutex);
            incrementVote(*partyList, party, mutex);
            //printParties(*partyList, mutex);                                                  //this prints votes to terminal, showcases every vote is registered properly one by one

            char* message = "Vote recieved\n";
            write(newsock, message, strlen(message));

            fprintf(file, "%s %s %s\n", name, lastName, party);
            fflush(file);

            pthread_mutex_unlock(mutex);

            close(newsock);

        } else if (bytes_read == 0) {
            continue;
        } else {
            continue;
        }
    }
}

void perror_exit(char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void sigchld_handler(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void sigint_handler(int sig) {
    interrupted = 1;
}
