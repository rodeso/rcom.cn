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


#define MAX_LENGTH 512
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
void handle_error(ErrorCode code, const char *msg) {
    // Error messages corresponding to ErrorCode
    const char *error_messages[] = {
        "Success",
        "Socket creation or connection failed",
        "Authentication failed",
        "Failed to enter passive mode",
        "Failed to retrieve file",
        "Failed to save file",
        "URL parsing failed"
    };

    // Print the error code and message
    fprintf(stderr, "Error [%d]: %s\n", code, error_messages[code]);
    if (msg) {
        fprintf(stderr, "Details: %s\n", msg);
    }

    // Clean up or exit if necessary
    exit(code); // Exits with the corresponding error code
}


// Parse the URL into its components
int parse_url(const char *input, URL *url) {
    // Initialize default user and password
    strcpy(url->user, "anonymous");
    strcpy(url->password, "anonymous");

    // Check for the "ftp://" prefix
    if (strncmp(input, "ftp://", 6) != 0) {
        handle_error(ERR_PARSE_FAIL, "Invalid URL");
    }

    const char *url_start = input + 6; // Skip "ftp://"
    const char *at_sign = strchr(url_start, '@');
    const char *slash = strchr(url_start, '/');

    if (!slash) {
        handle_error(ERR_PARSE_FAIL, "No path found.");
    }

    // Parse user and password if present
    if (at_sign && at_sign < slash) {
        const char *colon = strchr(url_start, ':');
        if (colon && colon < at_sign) {
            // Extract user and password
            strncpy(url->user, url_start, colon - url_start);
            strncpy(url->password, colon + 1, at_sign - colon - 1);
        } else {
            // No password provided
            strncpy(url->user, url_start, at_sign - url_start);
        }
        url_start = at_sign + 1; // Move past '@'
    }

    // Extract host
    strncpy(url->host, url_start, slash - url_start);

    // Extract resource
    strcpy(url->resource, slash + 1);

    // Extract file name from resource
    const char *last_slash = strrchr(url->resource, '/');
    if (last_slash) {
        strcpy(url->file, last_slash + 1);
    } else {
        strcpy(url->file, url->resource);
    }

    // Resolve hostname to IP
    struct hostent *host_info = gethostbyname(url->host);
    if (!host_info) {
       handle_error(ERR_PARSE_FAIL, "Failed to resolve hostname.");
    }
    strcpy(url->ip, inet_ntoa(*(struct in_addr *)host_info->h_addr));

    return 0; // Success
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
    memset(buffer, 0, buffer_size); // Clear buffer
    int bytes_received = recv(sockfd, buffer, buffer_size - 1, 0); // Read response
    if (bytes_received < 0) {
        perror("recv");
        return -1;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the response
    return bytes_received;
}

// Authenticate the user with the server
int authenticate(int sockfd, char *user, char *password) {
    char buffer[512];

    // Send USER command
    sprintf(buffer, "USER %s\r\n", user);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send USER");
        return -1;
    }

    // Read USER response
    if (read_response(sockfd, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "Failed to read response for USER command\n");
        return -1;
    }
    printf("USER Response: %s\n", buffer);

    // Check for "331" code (Password required)
    if (strstr(buffer, "331") == NULL) {
        fprintf(stderr, "Unexpected USER response: %s\n", buffer);
        return -1;
    }

    // Send PASS command
    sprintf(buffer, "PASS %s\r\n", password);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send PASS");
        return -1;
    }

    // Read PASS response
    if (read_response(sockfd, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "Failed to read response for PASS command\n");
        return -1;
    }
    printf("PASS Response: %s\n", buffer);

    // Check for "230" code (Login successful)
    if (strstr(buffer, "230") == NULL) {
        fprintf(stderr, "Authentication failed: %s\n", buffer);
        return -1;
    }

    return 0; // Authentication successful
}



// Enter passive mode and get the IP and port for data transfer
int enter_passive_mode(int sockfd, char *ip, int *port) {
    char buffer[MAX_LENGTH];
    send(sockfd, "PASV\r\n", 6, 0);
    recv(sockfd, buffer, sizeof(buffer), 0);

    int h1, h2, h3, h4, p1, p2;
    if (sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6) {
        sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
        *port = p1 * 256 + p2;
        return ERR_SUCCESS;
    }
    return ERR_PASV_FAIL;
}

// Request the file from the server
int request_file(int sockfd, char *resource) {
    char buffer[MAX_LENGTH];
    sprintf(buffer, "RETR %s\r\n", resource);
    send(sockfd, buffer, strlen(buffer), 0);
    recv(sockfd, buffer, sizeof(buffer), 0);
    return strstr(buffer, "150") ? ERR_SUCCESS : ERR_RETR_FAIL;
}

// Download the file from the server
int download_file(int sockfd, char *file) {
    FILE *fp = fopen(file, "wb");
    if (!fp) return ERR_FILE_SAVE_FAIL;

    char buffer[1024];
    int bytes_received;
    long total_bytes = 0;

    // Spinner characters
    char spinner[] = "|/-\\";
    int spinner_index = 0;

    printf("Downloading...\n");

    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
        total_bytes += bytes_received;

        // Display progress with spinner and bytes transferred
        printf("\r[%c] %ld bytes downloaded", spinner[spinner_index], total_bytes);
        fflush(stdout); // Force update of the console

        // Update spinner
        spinner_index = (spinner_index + 1) % 4;
    }
    fclose(fp);

    // Complete the progress bar
    printf("\r[✔] %ld bytes downloaded\n", total_bytes);

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
        close(control_socket); // Close control socket in case of authentication failure
        handle_error(ERR_AUTH_FAIL, "Authentication failed.");
    }

    char pasv_ip[MAX_LENGTH];
    int pasv_port;
    if (enter_passive_mode(control_socket, pasv_ip, &pasv_port) != ERR_SUCCESS) {
        close(control_socket); // Close control socket in case of passive mode failure
        handle_error(ERR_PASV_FAIL, "Failed to enter passive mode.");
    }

    int data_socket = create_socket(pasv_ip, pasv_port);
    if (data_socket < 0) {
        close(control_socket); // Close control socket before exiting
        handle_error(ERR_SOCKET_FAIL, "Failed to establish data connection.");
    }

    if (request_file(control_socket, url.resource) != ERR_SUCCESS) {
        close(control_socket); // Close both sockets
        close(data_socket);
        handle_error(ERR_RETR_FAIL, "Failed to request file.");
    }

    if (download_file(data_socket, url.file) != ERR_SUCCESS) {
        close(control_socket); // Close both sockets
        close(data_socket);
        handle_error(ERR_FILE_SAVE_FAIL, "Failed to save file.");
    }

    // Send QUIT command to the server before closing control socket
    send(control_socket, "QUIT\r\n", 6, 0);
    char buffer[512];
    recv(control_socket, buffer, sizeof(buffer), 0);
    printf("Server response: %s\n", buffer);

    // Close sockets
    close(data_socket);
    close(control_socket);

    printf("File '%s' downloaded successfully.\n", url.file);
    return ERR_SUCCESS;
}

