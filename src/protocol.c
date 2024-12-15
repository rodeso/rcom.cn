#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Constants
#define FTP_PORT 21
#define MAX_LENGTH 512

// ANSI color codes for colored output
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

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


// Error handling and messages
void handle_error(ErrorCode code, const char *msg) {
    const char *error_messages[] = {
        "Success",
        "Socket creation or connection failed",
        "Authentication failed",
        "Failed to enter passive mode",
        "Failed to retrieve file",
        "Failed to save file",
        "URL parsing failed"
    };

    fprintf(stderr, RED "Error [%d]: %s\n" RESET, code, error_messages[code]);
    if (msg) {
        fprintf(stderr, "Details: " YELLOW "%s\n" RESET, msg);
    }

    exit(code);
}


// Function to print parsed URL details
void print_url_info(const URL *url) {
    printf("\n");
    printf(CYAN "URL Details:\n" RESET);
    printf("User: " GREEN "%s\n" RESET, url->user);
    printf("Password: " GREEN "%s\n" RESET, url->password);
    printf("Host: " YELLOW "%s\n" RESET, url->host);
    printf("Resource Path: " BLUE "%s\n" RESET, url->resource);
    printf("File Name: " MAGENTA "%s\n" RESET, url->file);
    printf("Server IP: " WHITE "%s\n" RESET, url->ip);
    printf("\n");
}


// Parse the URL into its components
int parse_url(const char *input, URL *url) {
    memset(url->user, 0, MAX_LENGTH);
    memset(url->password, 0, MAX_LENGTH);

    if (strncmp(input, "ftp://", 6) != 0) {
        handle_error(ERR_PARSE_FAIL, "Invalid URL format.");
    }

    const char *url_start = input + 6;
    const char *at_sign = strchr(url_start, '@');
    const char *slash = strchr(url_start, '/');

    if (!slash) {
        handle_error(ERR_PARSE_FAIL, "Invalid URL format.");
    }
    if (at_sign && at_sign < slash) {
        const char *colon = strchr(url_start, ':');
        if (colon && colon < at_sign) {
            strncpy(url->user, url_start, colon - url_start);
            strncpy(url->password, colon + 1, at_sign - colon - 1);
        } else {
            strncpy(url->user, url_start, at_sign - url_start);
        }
        url_start = at_sign + 1;
    } else {
        strcpy(url->user, "anonymous");
        strcpy(url->password, "anonymous");
    }

    strncpy(url->host, url_start, slash - url_start);
    strcpy(url->resource, slash + 1);
    const char *last_slash = strrchr(url->resource, '/');
    if (last_slash) {
        strcpy(url->file, last_slash + 1);
    } else {
        strcpy(url->file, url->resource);
    }

    struct hostent *host_info = gethostbyname(url->host);
    if (!host_info) {
        handle_error(ERR_PARSE_FAIL, "Failed to resolve hostname.");
    }
    strcpy(url->ip, inet_ntoa(*(struct in_addr *)host_info->h_addr));

    return 0;
}


// Create a socket and connect to the serve
int create_socket(char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return ERR_SOCKET_FAIL;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return ERR_SOCKET_FAIL;
    }
    return sockfd;
}


// Read the server response
int read_response(int sockfd, char *buffer, size_t buffer_size) {
    memset(buffer, 0, buffer_size);
    char temp[MAX_LENGTH];
    int total_bytes = 0;
    int is_multiline = 0;
    char code[4] = {0};

    while (1) {
        int bytes_received = recv(sockfd, temp, sizeof(temp) - 1, 0);
        if (bytes_received < 0) {
            perror("recv");
            handle_error(ERR_SOCKET_FAIL, "Failed to read server response.");
        }
        temp[bytes_received] = '\0';
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
        total_bytes += bytes_received;
        if (total_bytes == bytes_received) {
            strncpy(code, buffer, 3);
            code[3] = '\0';
            if (buffer[3] == '-') {
                is_multiline = 1; 
            }
        }

        char *last_line = strrchr(buffer, '\n');
        if (last_line) {
            if (!is_multiline && strncmp(last_line + 1, code, 3) == 0) {
                break;
            } else if (is_multiline && strncmp(last_line + 1, code, 3) == 0 && last_line[4] == ' ') {
                break;
            }
        }
    }
    return total_bytes;
}


// Authenticate the user with the server
int authenticate(int sockfd, char *user, char *password) {
    char buffer[MAX_LENGTH];

    while (1) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("recv welcome message");
            return ERR_AUTH_FAIL;
        }
        buffer[bytes_received] = '\0';
        printf(CYAN "Welcome Message:\n" RESET "%s\n", buffer);

        if (strstr(buffer, "220 ") != NULL) {
            break;
        }
    }

    sprintf(buffer, "USER %s\r\n", user);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send USER");
        return ERR_AUTH_FAIL;
    }

    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv USER response");
        return ERR_AUTH_FAIL;
    }
    buffer[bytes_received] = '\0';
    printf(CYAN "USER Response:\n" RESET "%s\n", buffer);

    if (strstr(buffer, "331") == NULL) {
        fprintf(stderr, "Unexpected USER response: %s\n", buffer);
        return ERR_AUTH_FAIL;
    }

    sprintf(buffer, "PASS %s\r\n", password);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send PASS");
        return ERR_AUTH_FAIL;
    }

    bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv PASS response");
        return ERR_AUTH_FAIL;
    }
    buffer[bytes_received] = '\0';
    printf(CYAN "PASS Response:\n" RESET "%s\n", buffer);

    // LOGIN SUCCESSFUL
    if (strstr(buffer, "230") == NULL) {
        fprintf(stderr, "Authentication failed: %s\n", buffer);
        return ERR_AUTH_FAIL;
    }

    return ERR_SUCCESS;
}


// Enter passive mode and get the IP and port for data transfer
int enter_passive_mode(int sockfd, char *ip, int *port) {
    char buffer[MAX_LENGTH];

    if (send(sockfd, "PASV\r\n", strlen("PASV\r\n"), 0) < 0) {
        perror("send PASV");
        return ERR_PASV_FAIL;
    }

    while (1) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("recv PASV response");
            return ERR_PASV_FAIL;
        }
        buffer[bytes_received] = '\0';
        printf(CYAN "PASV Response:\n" RESET "%s\n", buffer);

        if (strstr(buffer, "227") != NULL) {
            int h1, h2, h3, h4, p1, p2;
            if (sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
                sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
                *port = (p1 * 256) + p2;
                printf(CYAN "Passive Mode Port:\n" RESET "%d\n", (p1 * 256) + p2);
                printf("\n");
                return ERR_SUCCESS;
            } else {
                fprintf(stderr, "Failed to parse PASV response: %s\n", buffer);
                return ERR_PASV_FAIL;
            }
        }
    }
}


// Request the file from the server
int request_file(int sockfd, char *resource) {
    char buffer[MAX_LENGTH];
    sprintf(buffer, "RETR %s\r\n", resource);
    send(sockfd, buffer, strlen(buffer), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    printf(CYAN "Server response:\n" RESET "%s\n", buffer);
    return strstr(buffer, "150") || strstr(buffer, "125") ? ERR_SUCCESS : ERR_RETR_FAIL;
}


// Download the file from the server
int download_file(int sockfd, char *file) {
    FILE *fp = fopen(file, "wb");
    if (!fp) return ERR_FILE_SAVE_FAIL;

    char buffer[MAX_LENGTH];
    ssize_t bytes_received;
    long total_bytes = 0;

    // Spinny
    char spinner[] = "|/-\\";
    int spinner_index = 0;

    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
        total_bytes += bytes_received;

        printf("\r[" YELLOW "%c" RESET "] %ld bytes downloaded", spinner[spinner_index], total_bytes);
        fflush(stdout);

        spinner_index = (spinner_index + 1) % 4;
    }
    fclose(fp);

    // CHECK
    printf("\r[" GREEN "✔" RESET "] %ld bytes downloaded\n", total_bytes);
    printf("\n");

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

    print_url_info(&url);

    int control_socket = create_socket(url.ip, FTP_PORT);
    if (control_socket < 0) {
        handle_error(ERR_SOCKET_FAIL, "Failed to establish control connection.");
    }

    if (authenticate(control_socket, url.user, url.password) != ERR_SUCCESS) {
        close(control_socket);
        handle_error(ERR_AUTH_FAIL, "Authentication failed.");
    }

    char pasv_ip[MAX_LENGTH];
    int pasv_port;
    if (enter_passive_mode(control_socket, pasv_ip, &pasv_port) != ERR_SUCCESS) {
        close(control_socket);
        handle_error(ERR_PASV_FAIL, "Failed to enter passive mode.");
    }

    int data_socket = create_socket(pasv_ip, pasv_port);
    if (data_socket < 0) {
        close(control_socket); 
        handle_error(ERR_SOCKET_FAIL, "Failed to establish data connection.");
    }

    if (request_file(control_socket, url.resource) != ERR_SUCCESS) {
        close(control_socket);
        close(data_socket);
        handle_error(ERR_RETR_FAIL, "Failed to request file.");
    }

    if (download_file(data_socket, url.file) != ERR_SUCCESS) {
        close(control_socket);
        close(data_socket);
        handle_error(ERR_FILE_SAVE_FAIL, "Failed to save file.");
    }

    send(control_socket, "QUIT\r\n", 6, 0);
    char buffer[MAX_LENGTH];
    recv(control_socket, buffer, sizeof(buffer), 0);
    printf(CYAN "Server response:\n" RESET "%s\n", buffer);

    close(data_socket);
    close(control_socket);

    printf(GREEN "File '%s' downloaded successfully.\n" RESET , url.file);
    return ERR_SUCCESS;
}
