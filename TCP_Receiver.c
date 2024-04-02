#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>  // Include this header for TCP options
#include <netinet/in.h>
#include "LinkedList.h"

#define MAX_BUFFER_SIZE 2048  

//  a function to calculate milliseconds
double get_time_in_milliseconds(struct timeval start, struct timeval end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {
    
    // *** Pre-Parts : Get from the user the command from terminal ***

    if (argc != 5) {        // ./TCP_Receiver -p 12345 -algo cubic
        fprintf(stderr, "Please provide the correct usage for the program: %s -ip IP -p PORT -algo ALGO <FILE_PATH>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract command-line arguments
    int port = 0;
    const char *congestion_algo = NULL;   // congestion_algo: The congestion control algorithm to be set (e.g., "reno" or "cubic").

    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]); // Convert a string representing an integer (ASCII string) to an integer value.
            i++;
        } else if (strcmp(argv[i], "-algo") == 0 && i + 1 < argc) {
            congestion_algo = argv[i + 1];
            i++;
        }
    }

    // Check if required arguments are provided
    if (port == 0 || congestion_algo == NULL) {
        fprintf(stderr, "Invalid arguments. Please provide the correct usage.\n");
        exit(EXIT_FAILURE);
    }
    
    // *** Part A: Create a TCP socket ***

    int listeningSocket = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket for IPv4
    if (listeningSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set congestion control algorithm
    // setsockopt system call to configure the TCP_CONGESTION option.
    // After that the specified congestion control algorithm is applied to the TCP connection.
    if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo)) != 0) {
        perror("Error setting congestion control algorithm");
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }

    // Define Receiver's values
    printf("Starting Receiver...\n");
    struct sockaddr_in server_address; // Struct sockaddr_in is defined in the <netinet/in.h> header file.
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // The server will listen on all available network interfaces.
    server_address.sin_port = htons(port); // htons() function ensures that the port number is properly formatted for network communication, regardless of the byte order of the host machine.

    // Bind the socket to the specified address and port
    if (bind(listeningSocket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
   if (listen(listeningSocket, 1) == -1) {
        perror("listen() failed");
        close(listeningSocket);
        exit(EXIT_FAILURE);
   } // listen to just one TCP connection 
    
    // *** Part B: Get a connection from the sender ***

    // Define Sender
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    memset(&client_address, 0, sizeof(client_address));

    // Accept a connection
    printf("Waiting for TCP connection...\n");

    int clientSocket = accept(listeningSocket, (struct sockaddr *)&client_address, &client_address_len);
    if (clientSocket == -1) {
        perror("Accept failed");
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }
    printf("Sender connected, beginning to receive file...\n");

    // *** Part C: Receive the file, measure the time it took to save ***

    struct timeval start_time, end_time;
    fileList* files = fileList_alloc(); // Start a new null list of files 
    char buffer[MAX_BUFFER_SIZE];
    
    // Receive the size of the file from the sender
    unsigned int file_size;
    ssize_t size_received = recv(clientSocket, &file_size, sizeof(file_size), 0);
    if (size_received < 0) {
        perror("Error receiving file size");
        close(clientSocket);
        close(listeningSocket);
        exit(EXIT_FAILURE);
    }

    // Receive the data of the file from the sender 
    while (1)
    {
        int total_received = 0;
        while(total_received<file_size) {
            gettimeofday(&start_time,NULL);
            ssize_t bytes_received = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
            if (bytes_received < 0) {
                perror("Error receiving file");
                close(clientSocket);
                close(listeningSocket);
                exit(EXIT_FAILURE);
            }
            // Connection closed
            if (bytes_received == 0) {
                printf("Connection closed by peer.\n");
                close(clientSocket);
                close(listeningSocket);
                exit(EXIT_SUCCESS);
            }
            total_received+=bytes_received;
        }
    
        gettimeofday(&end_time, NULL); // Set the end time before measuring elapsed time
        double elapsed_time = get_time_in_milliseconds(start_time, end_time);
        fileList_insertLast (files,elapsed_time,file_size); // Create new file 
        printf("File transfer completed (%d bytes).\n",total_received);
        printf("Waiting for Sender response...\n");
        memset(buffer, 0, strlen(buffer));

        // ** Part D: Wait for Sender Response **
        
        ssize_t decision = recv(clientSocket, buffer, 1, 0);
        if (decision < 0) {
            perror("Error receiving file");
            close(clientSocket);
            close(listeningSocket);
            exit(EXIT_FAILURE);
        }
        // if user don't want to send the file again, the next time in the loop won't measure time and will get exit message
        if(buffer[0]=='n') {
            memset(buffer, 0, strlen(buffer));
            break;
        }
        if (decision == 0) {
            printf("Connection closed by peer.\n");
            close(clientSocket);
            close(listeningSocket);
            exit(EXIT_SUCCESS);
        }
    }   
    
    // Get Exit message from Sender
    recv(clientSocket, buffer, 5, 0);
    if(buffer[1]=='E') {
        printf("Sender sent exit message.\n");
    }
    memset(buffer, 0, MAX_BUFFER_SIZE);

    // ** Part E: Print out statistics **
    
    printf("----------------------------------\n");
    printf("- * Statistics * -\n");
    fileList_print(files); //fucntion that prints time and speed for each file

    // ** Part F: Calculate the average time and the total average bandwith **
    
    fileList_AverageT_print(files);
    fileList_AverageBT_print(files);
    printf("----------------------------------\n");
    close(clientSocket);
    close(listeningSocket);
    fileList_free(files);
    
    // ** Part G: Exit **

    printf("Receicer end.\n");
    return 0;
 }
