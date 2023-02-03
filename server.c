#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PORT 15635

// content types to put in http header
char *content_types[7] = {
    "text/html",
    "text/plain",
    "image/png",
    "image/jpeg",
    "application/pdf",
    "image/x-icon",
    "application/octet-stream"};

void get_file_name_requested(char *http_request, char *file_name, char *content_type_name)
{
    
    // we expect only GET requests, that should be formatted GET filename HTTP-version \r\n {other stuff}
    if (!(http_request[0] == 'G' && http_request[1] == 'E' && http_request[2] == 'T' && http_request[3] == ' ' && http_request[4] == '/'))
    {
        // something we don't expect has occured
        //printf("client request was not of the expected format");
        if (http_request[0] == '0')
        {
            //printf("But the request was empty, so we won't exit");
        }
        else
        {
            //exit(1);
        }
    }
    
    const int first_relevent_idx = 5;
    int i = 0;
    bool seen_dot = false;
    int j = 0;
    int space_count = 0;

    while (http_request[i + first_relevent_idx + space_count * 2] != ' ')
    {

        if (seen_dot)
        {
            content_type_name[j] = http_request[i + first_relevent_idx + space_count * 2];
            j++;
        }

        if (http_request[i + first_relevent_idx + space_count * 2] == '%' && http_request[i + first_relevent_idx + 1 + space_count * 2] == '2' && http_request[i + first_relevent_idx + 2 + space_count * 2] == '0')
        {
            file_name[i] = ' ';
            space_count += 1;
        }
        else
        {
            //printf("here!");
            file_name[i] = http_request[i + first_relevent_idx + space_count * 2];
        }

        if (http_request[i + first_relevent_idx + space_count * 2] == '.')
        {
            seen_dot = true;
            sprintf(content_type_name, "");
            j = 0;
        }

        i++;
        printf("%i:%c,s:%s\n", i + first_relevent_idx, http_request[i + first_relevent_idx + space_count * 2], file_name);
    }

    // serve default requests
    if (i == 0)
    {
        // sprintf(file_name, "index.html");
        // sprintf(content_type_name, "html");
    }
    else
    {
        // null terminate
        file_name[i] = 0;
        if (j != 0)
        {
            content_type_name[j] = 0;
        }
    }

    printf("\nFILENAME REQUESTED: %s, CONTENT_TYPE_NAME:%s , I: %i\n", file_name, content_type_name, i);
}

char *canonicalize_content_type(char *content_type_name)
{
    if (strcmp(content_type_name, "html") == 0)
    {
        return content_types[0];
    }
    else if (strcmp(content_type_name, "txt") == 0)
    {
        return content_types[1];
    }
    else if (strcmp(content_type_name, "png") == 0)
    {
        return content_types[2];
    }
    else if (strcmp(content_type_name, "jpg") == 0)
    {
        return content_types[3];
    }
    else if (strcmp(content_type_name, "pdf") == 0)
    {
        return content_types[4];
    }
    else if (strcmp(content_type_name, "ico") == 0)
    {
        return content_types[5];
    }
    else if (strcmp(content_type_name, "") == 0)
    {
        return content_types[6];
    }
    else /* default: */
    {
        return content_types[6];
        //printf("some unrecognized content type! has been requested: %s", content_type_name);
        //exit(1);
    }
}
int main(int argc, char *argv[])
{
    //* only declare server setting stuff out of loop
    // server socket descriptor
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // htons host-to-net short (16-bit) translation
    address.sin_family = AF_INET;         // iv4 (set to that, some internet prootcol)
    address.sin_addr.s_addr = INADDR_ANY; // set the ip address to any (which we can recieve from)
    address.sin_port = htons(PORT);       // translate the port to big-endien=16bit

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // options for the socket file descriptor
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        close(server_fd);
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind socket to the port and IP address

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        close(server_fd);
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // server starts listening and can have up to 3 max clients queued for backlog
    //*done with setup -- listening
    if (listen(server_fd, 3) < 0)
    {
        close(server_fd);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // *while loop to listen to requests
    while (1)
    {
        //*declare variables at the top of the loop because of overwriting fears
        FILE *file;                        // for user's file
        int client_socket = 0;                 // client socket
        char content_type_name[100] = {0}; // the type filename.{whatever} requested by client
        char buffer[4096] = {0};           // buffer for client HTTP request;
        char http_header[4096] = {0};      // header for http response
        char dbuffer[4096];                // for sending file-chunks
        char file_name[4096];      // the file_name requested by client
        int valread = 0;                   // valread is just number of bytes returned from read() which we will call later
        int file_size = 0;                 // client's file size

        //* accept a client connection and read the message into the buffer
        // address and addrlen are result arguments that will have the client's info
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        valread = read(client_socket, buffer, 4096);
        if (valread <= 0)
        {
            perror("socket read");
        }

        // we read in the client's HTTP request and then we will parse this for the file name

        printf("CLIENT REQUEST \n ================================\n%s\n", buffer); // print client message for debugging purposes

        //*parse the client's request
        get_file_name_requested(buffer, file_name, content_type_name);

        //* open the file requested by clients, meassure it's length
        printf("\nTRYING TO OPEN FILE: %s\n", file_name); // prints out http_header for debugging purposes
        file = fopen(file_name, "rb");
        if (file == NULL)
        { //! maybe here we would want to give a 404, etc
            // set http_header with our file size and our desired content type
            sprintf(http_header, "HTTP/1.1 404 Not Found\r\n");

            printf("%s", http_header); // prints out http_header for debugging purposes

            // we send client socket the HTTP header first; make sure to do strlen instead of size_of (spent some time trying to debug this)
            send(client_socket, http_header, strlen(http_header), 0);
            //fclose(file);
            close(client_socket);
            perror("fopen");
            continue;
        }

        // get the file size
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        rewind(file);

        //* build and send http header

        // set http_header with our file size and our desired content type
        sprintf(http_header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: %s\r\n\r\n", file_size, canonicalize_content_type(content_type_name));

        printf("%s", http_header); // prints out http_header for debugging purposes

        // we send client socket the HTTP header first; make sure to do strlen instead of size_of (spent some time trying to debug this)
        send(client_socket, http_header, strlen(http_header), 0);

        // we will send the file after in chunks of 4096 bytes
        // databuffer for reading and sending

        // in this chunk of code, we are checking if FILE* file descriptor is valid
        // then we will read in data from the file in chunks of 4096 bytes
        // after reading that data, we will send that chunk of data to our client socket
        // we keep doing this until the number of bytes read from the file is 0
        // if send returns -1, it is an error
        size_t bytes_read;
        if (file > 0 && file != NULL)
        {
            //printf("call in loop of msg recieve");
            memset(dbuffer, 0, 4096); // so we don't send something that is no longer file-- or whatever
            while ((bytes_read = fread(dbuffer, 1, 4096, file)) > 0)
            {
                if (send(client_socket, dbuffer, bytes_read, 0) < 0)
                {
                    close(client_socket);
                    perror("send:data-packets");
                    //return 1;
                }
            }
        }

        close(client_socket);
        fclose(file);
    }


    return 0;
}
