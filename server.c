#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#define PORT 15635

// content types to put in http header
char* content_types[6] = {
    "text/html", 
    "text/plain",
    "image/png",
    "image/jpg",
    "application/pdf",
    "application/octet-stream\r\nContent-Disposition: inline"
};

int main(int argc, char*argv[]) {

    // this part up to line 57 not really applicable to project but just for testing different content types
    // do command `./server filetype` with filetype being what filetype you want to show
    // ex: ./server png will show a png file
    // filetypes: html, txt, png, jpg, pdf bin
    // for right now, bin is not displaying in browser (might be due to client browser prevention/blocking)

    // for now, we are opening file before the server is setup
    // later, we will be opening file after the server is setup and after we get file name from client HTTP request 
    // so this will probably be in the while in line 144
    char file_name[1000] = "test";

    int content_type;
    // default
    if (argc!= 2) {
        strcat(file_name, "html");
        content_type = 0;
    }
    else {
        if (strcmp(argv[1],"html") == 0) {
            strcat(file_name, ".html");
            content_type = 0;
        }
        else if (strcmp(argv[1],"txt") == 0) {
            strcat(file_name, ".txt");
            content_type = 1;
        }
        else if (strcmp(argv[1],"png") == 0) {
            strcat(file_name, ".png");
            content_type = 2;
        }
        else if (strcmp(argv[1],"jpg") == 0) {
            strcat(file_name, ".jpg");
            content_type = 3;
        }
        else if (strcmp(argv[1],"pdf") == 0) {
            strcat(file_name, ".pdf");
            content_type = 4;
        }
        else if (strcmp(argv[1],"bin") == 0) {
            content_type = 5;
        }
        else {
            return 1;
        }
    }

    // opening file

    FILE* file;

    file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    // getting file size
    int file_size;

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

     // need http_header for proper HTTP response to client or there will be errors
    char http_header[4096];
    memset(http_header,0,4096);

    // set http_header with our file size and our desired content type
    sprintf(http_header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: %s\r\n\r\n", file_size, content_types[content_type]);

    // prints out http_header for debugging purposes
    printf("%s", http_header);    

    // server socket descriptor
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

    // buffer for client HTTP request; will need to parse this later to get the file name
    char buffer[1024] = {0};

    // valread is just number of bytes returned from read() which we will call later
    int valread;

    // while loop cause server will keep listening for requests
    while(1) {
        // address and addrlen are result arguments that will have the client's info
        // we redefined address and addrlen and we can probably use this later
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t * ) & addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        } 

        // we read in the client's HTTP request and then we will parse this for the file name
        valread = read(new_socket, buffer, 1024);

        // printing client request
        printf("%s\n", buffer);

        // we send client socket the HTTP header first; make sure to do strlen instead of size_of (spent some time trying to debug this)
        send(new_socket, http_header, strlen(http_header), 0);

        // we will send the file after in chunks of 4096 bytes
        // databuffer for reading and sending
        char dbuffer[4096];
        memset(dbuffer, 0, 4096);

        // in this chunk of code, we are checking if FILE* file descriptor is valid
        // then we will read in data from the file in chunks of 4096 bytes
        // after reading that data, we will send that chunk of data to our client socket
        // we keep doing this until the number of bytes read from the file is 0
        // if send returns -1, it is an error
        size_t bytes_read;
        if (file > 0) {
            while ((bytes_read = fread(dbuffer, 1, 4096, file)) > 0) {
                if (send(new_socket, dbuffer, bytes_read, 0) < 0) {
                    perror("Error\n");
                    return 1;
                }
            }
        }
        
        close(new_socket);
    }
    fclose(file);
    
    return 0;
}
