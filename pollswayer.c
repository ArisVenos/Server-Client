#include <stdio.h>
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <unistd.h> /* read, write, close */
#include <netdb.h> /* gethostbyaddr */
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */
#include <pthread.h> /* threads */
#include <fcntl.h> /* file operations */

void perror_exit(char *message);
void* thread_function(void* arg);

struct ThreadData {
    char* host;
    int port;
    char* vote;
};

void main(int argc, char *argv[]) {
    int port, sock, i;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    if (argc != 4) {
        printf("Please give host name, port number, and input file\n");
        exit(1);
    }

    /* Find server address */
    if ((rem = gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    port = atoi(argv[2]); /* Convert port number to integer */
    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr_list[0], rem->h_length);
    server.sin_port = htons(port); /* Server port */

    // Read input file
    FILE* file = fopen(argv[3], "r");
    if (file == NULL) {
        perror_exit("fopen");
    }

    // Count the number of lines in the file
    int num_lines = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            num_lines++;
        }
    }
    rewind(file); /* Reset file pointer to the beginning */

    // Create an array of threads
    pthread_t* threads = (pthread_t*)malloc(num_lines * sizeof(pthread_t));
    if (threads == NULL) {
        perror_exit("malloc");
    }

    // Create threads and establish connections
    for (int i = 0; i < num_lines; i++) {
        char vote[256];
        if (fgets(vote, sizeof(vote), file) == NULL) {
            perror_exit("fgets");
        }

        /* Remove the newline character from the vote */
        size_t len = strlen(vote);
        if (len > 0 && vote[len - 1] == '\n') {
            vote[len - 1] = '\0';
        }

        // Create thread data
        struct ThreadData* thread_data = (struct ThreadData*)malloc(sizeof(struct ThreadData));
        if (thread_data == NULL) {
            perror_exit("malloc");
        }
        thread_data->host = argv[1];
        thread_data->port = port;
        thread_data->vote = strdup(vote);

        // Create thread
        if (pthread_create(&threads[i], NULL, thread_function, (void*)thread_data) != 0) {
            perror_exit("pthread_create");
        }
    }

    // Wait for threads to complete
    for (int i = 0; i < num_lines; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror_exit("pthread_join");
        }
    }

    printf("All votes completed.\n");

    // Cleanup
    fclose(file);
    close(sock);
    free(threads);

    exit(0);
}

void* thread_function(void* arg) {
    struct ThreadData* thread_data = (struct ThreadData*)arg;
    int port, sock;
    char buf[256];
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror_exit("socket");
    }

    /* Find server address */
    if ((rem = gethostbyname(thread_data->host)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    port = thread_data->port; /* Port number */
    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr_list[0], rem->h_length);
    server.sin_port = htons(port); /* Server port */

    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0) {
        perror_exit("connect");
    }

    printf("Connected to %s port %d\n", thread_data->host, port);

    // Read and print the first message from the server
    ssize_t bytes_read = read(sock, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        perror_exit("read");
    }
    buf[bytes_read] = '\0'; // Null-terminate the received message
    printf("%s", buf);

    // Send vote to the server
    size_t len = strlen(thread_data->vote);
    if (write(sock, thread_data->vote, len) != len) {
        perror_exit("write");
    }

    // Read and print the second message from the server
    bytes_read = read(sock, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        perror_exit("read");
    }
    buf[bytes_read] = '\0'; // Null-terminate the received message
    printf("%s", buf);

    close(sock);

    // Cleanup
    free(thread_data->vote);
    free(thread_data);

    pthread_exit(NULL);
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
