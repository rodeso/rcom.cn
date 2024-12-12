#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_LENGTH 1024
#define FTP_PORT 21

// URL format: ftp://[<user>:<password>@]<host>/<url-path>
typedef struct URL {
    char user[MAX_LENGTH];      // Username for authentication
    char password[MAX_LENGTH];  // Password for authentication
    char host[MAX_LENGTH];      // Hostname or IP address of the FTP server
    char resource[MAX_LENGTH];  // Path to the resource on the server
    char file[MAX_LENGTH];      // Name of the file to download
    char ip[MAX_LENGTH];        // IP address of the server
} URL;

// Centralized error codes
typedef enum {
    ERR_SUCCESS = 0,
    ERR_SOCKET_FAIL,
    ERR_AUTH_FAIL,
    ERR_PASV_FAIL,
    ERR_RETR_FAIL,
    ERR_FILE_SAVE_FAIL,
    ERR_PARSE_FAIL,
} ErrorCode;

// Enhanced error handling and messages
void handle_error(ErrorCode code, const char* msg) {
    fprintf(stderr, "Error [%d]: %s\n", code, msg);
    exit(code);
}

// Parse the URL into its components
int parse_url(const char* input, URL* url) {
    // Initialize the URL structure
    memset(url, 0, sizeof(URL));

    // Example: ftp://user:password@host/resource
    char temp[MAX_LENGTH];
    strncpy(temp, input, MAX_LENGTH - 1);

    // Extract user and password if provided
    char* at = strchr(temp, '@');
    if (at) {
        char* colon = strchr(temp, ':');
        if (colon && colon < at) {
            *colon = '\0';
            strncpy(url->user, temp + 6, MAX_LENGTH - 1); // Skip "ftp://"
            strncpy(url->password, colon + 1, MAX_LENGTH - 1);
        } else {
            strncpy(url->user, temp + 6, MAX_LENGTH - 1); // Default user
            strncpy(url->password, "anonymous", MAX_LENGTH - 1);
        }
        strncpy(temp, at + 1, MAX_LENGTH - 1); // Trim input to host/resource
    } else {
        strncpy(url->user, "anonymous", MAX_LENGTH - 1);
        strncpy(url->password, "anonymous", MAX_LENGTH - 1);
    }

    // Extract host and resource
    char* slash = strchr(temp, '/');
    if (slash) {
        *slash = '\0';
        strncpy(url->host, temp, MAX_LENGTH - 1);
        strncpy(url->resource, slash + 1, MAX_LENGTH - 1);
    } else {
        strncpy(url->host, temp, MAX_LENGTH - 1);
        url->resource[0] = '\0';
    }

    // Extract the file name from the resource
    char* file = strrchr(url->resource, '/');
    if (file) {
        strncpy(url->file, file + 1, MAX_LENGTH - 1);
    } else {
        strncpy(url->file, url->resource, MAX_LENGTH - 1);
    }

    // Resolve the IP address
    struct hostent* h;
    if ((h = gethostbyname(url->host)) == NULL) {
        fprintf(stderr, "Invalid hostname: %s\n", url->host);
        return -1;
    }
    strncpy(url->ip, inet_ntoa(*(struct in_addr*)h->h_addr), MAX_LENGTH - 1);

    printf("Host: %s\n", url->host);
    printf("User: %s\n", url->user);
    printf("Password: %s\n", url->password);
    printf("Resource: %s\n", url->resource);

    return 0;
}

// Create a socket and connect to the serve
int create_socket(char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Clear and set up the server address structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);  
    server_addr.sin_port = htons(port); 
    
    // Create the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    
    return sockfd;
}

// Authenticate the user with the server
int authenticate(int sockfd, char *user, char *password) {
    char buffer[MAX_LENGTH];

    // Read the welcome message
    read(sockfd, buffer, MAX_LENGTH);
    printf("%s", buffer);

    // Send the username
    sprintf(buffer, "USER %s\r\n", user);
    write(sockfd, buffer, strlen(buffer));
    read(sockfd, buffer, MAX_LENGTH);
    printf("%s", buffer);

    // Send the password
    sprintf(buffer, "PASS %s\r\n", password);
    write(sockfd, buffer, strlen(buffer));
    read(sockfd, buffer, MAX_LENGTH);
    printf("%s", buffer);

    return ERR_SUCCESS;
}

// Enter passive mode and get the IP and port for data transfer
int enter_passive_mode(int sockfd, char *ip, int *port) {
    char buffer[MAX_LENGTH];
    char *token;
    int i, p1, p2;

    // Send the PASV command
    sprintf(buffer, "PASV\r\n");
    write(sockfd, buffer, strlen(buffer));
    int bytes = read(sockfd, buffer, MAX_LENGTH);
    if (bytes < 0) {
        perror("Error reading PASV response");
        return -1;
    }
    buffer[bytes] = '\0';  // Null-terminate the string
    printf("PASV Response: %s\n", buffer);  // Debug output

    // Extract the IP and port from the PASV response
    token = strtok(buffer, "(");
    token = strtok(NULL, "(");
    for (i = 0; i < 4; i++) {
        token = strtok(NULL, ",");
    }

    sprintf(ip, "%s.%s.%s.%s", token, strtok(NULL, ","), strtok(NULL, ","), strtok(NULL, ","));
    p1 = atoi(strtok(NULL, ","));
    p2 = atoi(strtok(NULL, ","));
    *port = p1 * 256 + p2;

    printf("PASV IP: %s, Port: %d\n", ip, *port);  // Debug output

    return ERR_SUCCESS;
}


// Request the file from the server
int request_file(int sockfd, char *resource) {
    char buffer[MAX_LENGTH];

    // Send the RETR command
    sprintf(buffer, "RETR %s\r\n", resource);
    write(sockfd, buffer, strlen(buffer));
    read(sockfd, buffer, MAX_LENGTH);
    printf("%s", buffer);

    return ERR_SUCCESS;
}

// Download the file from the server
int download_file(int sockfd, char *file) {
    FILE *fp;
    char buffer[MAX_LENGTH];
    int bytes;

    // Open the file for writing
    fp = fopen(file, "w");
    if (fp == NULL) {
        perror("fopen()");
        return -1;
    }

    // Read and write the file contents
    while ((bytes = read(sockfd, buffer, MAX_LENGTH)) > 0) {
        fwrite(buffer, 1, bytes, fp);
    }

    fclose(fp);
    return ERR_SUCCESS;
}

// Main function to download a file from an FTP server
int main(int argc, char *argv[]) {
    if (argc != 2) {
        handle_error(ERR_PARSE_FAIL, "Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>");
    }

    struct URL url;
    if (parse_url(argv[1], &url) != 0) {
        handle_error(ERR_PARSE_FAIL, "Failed to parse URL.");
    }

    printf("Connecting to %s (%s)...\n", url.host, url.ip);

    int control_socket = create_socket(url.ip, FTP_PORT);
    if (control_socket < 0) {
        handle_error(ERR_SOCKET_FAIL, "Failed to establish control connection.");
    }

    if (authenticate(control_socket, url.user, url.password) != ERR_SUCCESS) {
        handle_error(ERR_AUTH_FAIL, "Authentication failed.");
    }

    char pasv_ip[MAX_LENGTH];
    int pasv_port;
    if (enter_passive_mode(control_socket, pasv_ip, &pasv_port) != ERR_SUCCESS) {
        handle_error(ERR_PASV_FAIL, "Failed to enter passive mode.");
    }

    int data_socket = create_socket(pasv_ip, pasv_port);
    if (data_socket < 0) {
        handle_error(ERR_SOCKET_FAIL, "Failed to establish data connection.");
    }

    if (request_file(control_socket, url.resource) != ERR_SUCCESS) {
        handle_error(ERR_RETR_FAIL, "Failed to request file.");
    }

    if (download_file(data_socket, url.file) != ERR_SUCCESS) {
        handle_error(ERR_FILE_SAVE_FAIL, "Failed to save file.");
    }

    printf("File '%s' downloaded successfully.\n", url.file);

    close(control_socket);
    close(data_socket);

    return ERR_SUCCESS;
}
