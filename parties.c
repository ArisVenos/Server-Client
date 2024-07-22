#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include "poller.h"


void addParty(struct Party** partyList, const char* partyName, pthread_mutex_t* mutex) {
    struct Party* party = *partyList;
    while (party != NULL) {
        if (strcmp(party->name, partyName) == 0) {
            return;
        }
        party = party->next;
    }

    struct Party* newParty = (struct Party*)malloc(sizeof(struct Party));
    if (newParty == NULL) {
        perror_exit("malloc");
    }

    strcpy(newParty->name, partyName);
    newParty->votes = 0;
    newParty->next = *partyList;

    *partyList = newParty;
    //printf("added party: %s\n",newParty->name);

    return;
}

void incrementVote(struct Party* partyList, const char* partyName, pthread_mutex_t* mutex) {
    struct Party* party = partyList;
    while (party != NULL) {
        if (strcmp(party->name, partyName) == 0) {
            party->votes++;
            printf("Vote for %s registered\n",party->name);
            return;
        }
        party = party->next;
    }
}

void printParties(struct Party* partyList, pthread_mutex_t* mutex) {
    struct Party* party = partyList;

    printf("Party\t\tVotes\n");
    printf("=====================\n");

    while (party != NULL) {
        printf("%s\t\t%d\n", party->name, party->votes);
        party = party->next;
    }

    printf("=====================\n");
}

void fprintParties(struct Party* partyList, FILE* file) {
    struct Party* party = partyList;

    fprintf(file,"Party\t\tVotes\n");
    fprintf(file,"=====================\n");

    while (party != NULL) {
        fprintf(file,"%s\t\t%d\n", party->name, party->votes);
        party = party->next;
    }

    fprintf(file,"=====================\n");
}

void freeParties(struct Party* partyList) {
    struct Party* party = partyList;
    while (party != NULL) {
        struct Party* nextParty = party->next;
        free(party);
        party = nextParty;
    }
}