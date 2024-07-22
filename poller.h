#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <pthread.h>

#define MAX_VOTES 100

struct Party {
    char name[1024];
    int votes;
    struct Party* next;
};

struct Connection {
    int newsock;
    struct Connection* next;
};

struct ConnectionBuffer {
    struct Connection* head;
    struct Connection* tail;
    int count;
};

struct ThreadArgs {
    int newsock;
    const char* filename;
    struct Party** partyList;
    pthread_mutex_t* mutex;
    pthread_cond_t* bufferNotEmpty;
    pthread_cond_t* bufferNotFull;
    struct ConnectionBuffer* connectionBuffer;
    int threadid;
};

void* child_server(void* arg);
void addParty(struct Party** partyList, const char* partyName, pthread_mutex_t* mutex);
void incrementVote(struct Party* partyList, const char* partyName, pthread_mutex_t* mutex);
void printParties(struct Party* partyList, pthread_mutex_t* mutex);
void freeParties(struct Party* partyList);
void perror_exit(char* message);
void sigchld_handler(int sig);
void sigint_handler(int sig);
void fprintParties(struct Party* partyList, FILE* file); 

#endif