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
    // Initialize the user and password fields with empty strings
    memset(url->user, 0, MAX_LENGTH);
    memset(url->password, 0, MAX_LENGTH);

    // Check for the "ftp://" prefix
    if (strncmp(input, "ftp://", 6) != 0) {
        return -1; // Invalid URL
    }

    const char *url_start = input + 6; // Skip "ftp://"
    const char *at_sign = strchr(url_start, '@');
    const char *slash = strchr(url_start, '/');

    if (!slash) {
        return -1; // No path found
    }

    // Parse user and password if present
    if (at_sign && at_sign < slash) {
        const char *colon = strchr(url_start, ':');
        if (colon && colon < at_sign) {
            // Extract user and password
            strncpy(url->user, url_start, colon - url_start);
            strncpy(url->password, colon + 1, at_sign - colon - 1);
        } else {
            // No password provided, only extract user
            strncpy(url->user, url_start, at_sign - url_start);
        }
        url_start = at_sign + 1; // Move past '@'
    } else {
        // If no user/password specified, set them to "anonymous"
        strcpy(url->user, "anonymous");
        strcpy(url->password, "anonymous");
    }

    // Extract host (everything between '@' and '/')
    strncpy(url->host, url_start, slash - url_start);

    // Extract resource (path after '/')
    strcpy(url->resource, slash + 1);

    // Extract file name from resource (everything after the last '/')
    const char *last_slash = strrchr(url->resource, '/');
    if (last_slash) {
        strcpy(url->file, last_slash + 1);
    } else {
        strcpy(url->file, url->resource);
    }

    // Resolve hostname to IP
    struct hostent *host_info = gethostbyname(url->host);
    if (!host_info) {
        return -1; // Failed to resolve hostname
    }
    strcpy(url->ip, inet_ntoa(*(struct in_addr *)host_info->h_addr));

    return 0; // Success
}

// Function to print parsed URL details
void print_url_info(const URL* url) {
    printf("User: %s\n", url->user);
    printf("Password: %s\n", url->password);
    printf("Host: %s\n", url->host);
    printf("Resource Path: %s\n", url->resource);
    printf("File Name: %s\n", url->file);
    printf("Server IP: %s\n", url->ip);
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
    memset(buffer, 0, buffer_size); // Clear the buffer
    char temp[MAX_LENGTH];                // Temporary buffer for each line
    int total_bytes = 0;
    int is_multiline = 0;
    char code[4] = {0};

    while (1) {
        // Receive data from the server
        int bytes_received = recv(sockfd, temp, sizeof(temp) - 1, 0);
        if (bytes_received < 0) {
            perror("recv");
            return -1; // Error in receiving
        }
        temp[bytes_received] = '\0'; // Null-terminate the received data

        // Append the received data to the main buffer
        strncat(buffer, temp, buffer_size - strlen(buffer) - 1);
        total_bytes += bytes_received;

        // Determine if this is a multi-line response
        if (total_bytes == bytes_received) { // First recv call
            strncpy(code, buffer, 3); // Extract the response code
            code[3] = '\0';
            if (buffer[3] == '-') {
                is_multiline = 1; // Multi-line response detected
            }
        }

        // Check if the final line has been received
        char *last_line = strrchr(buffer, '\n');
        if (last_line) {
            if (!is_multiline && strncmp(last_line + 1, code, 3) == 0) {
                break; // Single-line response complete
            } else if (is_multiline && strncmp(last_line + 1, code, 3) == 0 && last_line[4] == ' ') {
                break; // Multi-line response complete
            }
        }
    }

    return total_bytes;
}


// Authenticate the user with the server
int authenticate(int sockfd, char *user, char *password) {
    char buffer[MAX_LENGTH];

    // Step 1: Read and discard the initial welcome message
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv welcome message");
        return ERR_AUTH_FAIL;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the response
    printf("Welcome Message: %s\n", buffer);

    // Step 2: Send USER command
    sprintf(buffer, "USER %s\r\n", user);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send USER");
        return ERR_AUTH_FAIL;
    }

    // Step 3: Read response to USER command
    bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv USER response");
        return ERR_AUTH_FAIL;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the response
    printf("USER Response: %s\n", buffer);

    // Check for "331" code (Password required)
    if (strstr(buffer, "331") == NULL) {
        fprintf(stderr, "Unexpected USER response: %s\n", buffer);
        return ERR_AUTH_FAIL;
    }

    // Step 4: Send PASS command
    sprintf(buffer, "PASS %s\r\n", password);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send PASS");
        return ERR_AUTH_FAIL;
    }

    // Step 5: Read response to PASS command
    bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv PASS response");
        return ERR_AUTH_FAIL;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the response
    printf("PASS Response: %s\n", buffer);

    // Check for "230" code (Login successful)
    if (strstr(buffer, "230") == NULL) {
        fprintf(stderr, "Authentication failed: %s\n", buffer);
        return ERR_AUTH_FAIL;
    }

    return ERR_SUCCESS; // Authentication successful
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
    printf("Server response: %s\n", buffer);
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

    // Print the parsed URL details
    print_url_info(&url);

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
    char buffer[MAX_LENGTH];
    recv(control_socket, buffer, sizeof(buffer), 0);
    printf("Server response: %s\n", buffer);

    // Close sockets
    close(data_socket);
    close(control_socket);

    printf("File '%s' downloaded successfully.\n", url.file);
    return ERR_SUCCESS;
}

