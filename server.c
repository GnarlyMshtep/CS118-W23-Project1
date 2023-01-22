#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#define PORT 15635

int main() {

    // reading html file and setting up HTTP response
    FILE* html_data;
    html_data =  fopen("test.html", "r");

    char response_data[1024];
    fgets(response_data, 1024, html_data);

    // need http_header for proper HTTP response to client or there will be errors
    char http_header[2048] = "HTTP/1.1 200 OK\r\n\n";
    strcat(http_header, response_data);
    
    // buffer for client HTTP request
    char buffer[1024] = {0};

    int server_fd;
    
    // defining socket's port and IP address
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // htons host-to-net short (16-bit) translation

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // options for the socket file descriptor
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind socket to the port and IP address

    if (bind(server_fd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // server starts listening and can have up to 3 max clients queued for backlog

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // client socket

    int new_socket;
    int valread;

    // while loop cause server will keep listening for requests
    while(1) {
        // address and addrlen are result arguments that will have the client's info
        // we redefined address and addrlen and we can probably use this later
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
                                (socklen_t * ) & addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        } 

        // we read in the client's HTTP request just for console output purposes

        valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);

        // we send client socket the HTTP response
        send(new_socket, http_header, sizeof(http_header), 0);
        close(new_socket);
    }

    printf("Hello message sent\n");

    
    return 0;
}
