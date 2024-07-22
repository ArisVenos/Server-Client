#include <stdio.h>
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <unistd.h> /* read, write, close */
#include <netdb.h> /* gethostbyaddr */
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */

void perror_exit(char *message);

void main(int argc, char *argv[]) {

    int port, sock, i;
    char buf1[256],buf2[256];
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    if (argc != 3) {
        printf("Please give host name and port number\n");
        exit(1);
    }

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror_exit("socket");
    }

    /* Find server address */
    if ((rem = gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname"); 
        exit(1);
    }

    port = atoi(argv[2]); /*Convert port number to integer*/
    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr_list[0], rem->h_length);
    server.sin_port = htons(port); /* Server port */

    // Initiate connection 

    if (connect(sock, serverptr, sizeof(server)) < 0) {
        perror_exit("connect");
    }
    
    //printf("made connection\n");

    ssize_t bytes_read;

    bytes_read = read(sock, buf1, sizeof(buf1) - 1);
    if (bytes_read < 0) {
        perror_exit("read");
    }

    buf1[bytes_read] = '\0'; // Null-terminate the received message

    printf(" %s", buf1);

    bytes_read = read(sock, buf2, sizeof(buf2) - 1);
    if (bytes_read < 0) {
        perror_exit("read");
    }

    buf2[bytes_read] = '\0'; // Null-terminate the received message

    printf(" %s", buf2);
    fgets(buf1, sizeof(buf1), stdin);
    
    size_t len = strlen(buf1);

    if (write(sock, buf1, len) != len) {
        perror_exit("write");
    }

    printf("Sent vote: %s", buf1);

    
    close(sock); /* Close socket and exit */
}   

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}