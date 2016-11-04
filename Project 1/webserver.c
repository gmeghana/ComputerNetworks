#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>   /* for the waitpid() system call */
#include <signal.h> 
#include <time.h> 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void read_request (char* url, int request) {
    // open url
    // if url does not exist, display error
    char response[1024];
    int resource;
    resource = open(++url, O_RDONLY);
    // checks if resource does not exist
    if (resource < 0) {
        char* not_found = 
        "HTTP/1.1 404 Not Found\n"
        "Content-type: text/html\n"
        "\n"
        "<html>\n"
        " <body>\n"
        "  <h1>Not Found</h1>\n"
        " </body>\n"
        "</html>\n";
        sprintf(response, not_found, url);
        write(request, response, strlen (response));
        error("ERROR opening file");
    }
    
    // get file extension
    char *file_ext = strrchr (url, '.'); 

    // match file extension to header
    char *header;
    file_ext = file_ext + 1;
    int html = strcasecmp(file_ext, "html");
    int gif = strcasecmp(file_ext, "gif");
    int jpg = strcasecmp(file_ext, "jpg");
    if (html == 0) {
        header = "text/html";
    }
    else if (gif == 0) {
        header = "image/gif";
    }
    else if (jpg == 0) {
        header = "image/jpeg";
    } 

    // display ok response
    char* ok_response =
    "HTTP/1.1 200 OK\n"
    "Content-type: %s\n"
    "\n";
    sprintf(response, ok_response, header);
    write(request, response, strlen (response));
    int num_bytes_read;
    char buffer;
    while ( (num_bytes_read = read(resource, &buffer, 1)) ) {
        if ( write(request, &buffer, 1) < 1 )
            error("ERROR sending file.");
        if ( num_bytes_read < 0 )
            error("ERROR reading from file.");
    }
}

void parse_request (int request) {
    // read in data 
    char buffer[1024];
    ssize_t num_bytes_read;
    num_bytes_read = read(request, buffer, sizeof(buffer)-1);
    if (num_bytes_read == 0) {
        // connection is closed 
    }
    else if (num_bytes_read > 0) {
        char method[sizeof(buffer)];
        char url[sizeof(buffer)];
        char protocol[sizeof(buffer)];
        buffer[num_bytes_read] = '\0';
        
        // client sends method, url, and protocol
        sscanf (buffer, "%s %s %s", method, url, protocol);
        printf("%s", buffer);

        if (strcmp(method, "GET")) {
            char response[1024];
            char* bad_method = 
            "HTTP/1.1 501 Method Not Implemented\n"
            "Content-type: text/html\n"
            "\n"
            "<html>\n"
            " <body>\n"
            "  <h1>Method Not Implemented</h1>\n"
            " </body>\n"
            "</html>\n";
            sprintf (response, bad_method, method);
            write (request, response, strlen(response));
        }
        else if (strcmp (protocol, "HTTP/1.1")) {
            char* bad_request = 
            "HTTP/1.1 400 Bad Request\n"
            "Content-type: text/html\n"
            "\n"
            "<html>\n"
            " <body>\n"
            "  <h1>Bad Request</h1>\n"
            " </body>\n"
            "</html>\n";
            write (request, bad_request, strlen (bad_request));
        }
        else
            // read in the request if there is no bad method or bad request 
            read_request(url, request);
    }
    else {
        error("ERROR reading request");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;    
    struct sockaddr_in serv_addr, cli_addr; 
    time_t ticks; 

    if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory
    char buffer[1025];
    memset(buffer, 0, sizeof(buffer)); 

    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

    listen(sockfd, 1);  //1 simultaneous connection at most

    // infinite loop to make sure server is constantly running
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, (unsigned int *) &clilen); 
        
        if (newsockfd < 0)
            error("ERROR on accept");

        // let each process have its own connection
        pid = fork();
        if (pid < 0) {
            error("ERROR on fork");
        }
        else if (pid == 0) {
            close(sockfd);
            parse_request(newsockfd);
            if (close(newsockfd) < 0) {
                error("ERROR closing connection");
            }
            exit(EXIT_SUCCESS);
        }
        else {
            if (close(newsockfd) < 0)
                error("ERROR closing connection");
        }
    }
}
