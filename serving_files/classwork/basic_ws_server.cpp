#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fstream>
#include <time.h>
#include <chrono>

static char buffer[4096];
void* handle_connection(void* socket_fd_ptr) {
    // cout << "Thread started, waiting 8s\n";
    // sleep(8);
    int socket_fd = *(int*)socket_fd_ptr;
    ssize_t bytes_read;

    std::string request;
    do {
        bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            request.append(buffer);
            if (request.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
    } while (bytes_read > 0);

    if (bytes_read == -1) {
        std::cerr << "Error reading request data." << std::endl;
        close(socket_fd);
        return nullptr;
    }

    std::cout << "Received HTTP request:\n" << request << std::endl;

    const char* response_body = "Hello, World!\n";
    char http_response[128];
    int body_length = strlen(response_body);
    int response_length = snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s", body_length, response_body);

    if (response_length >= 0) {
        write(socket_fd, http_response, ((size_t)response_length > sizeof(http_response)) ? sizeof(http_response) : (size_t)response_length);
        std::cout << "Sent HTTP response.\n";
    } else {
        std::cerr << "Error sending HTTP response." << std::endl;
    }

    buffer[0] = '\0';
    close(socket_fd);
    return nullptr;
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening on port 8080...\n";

    while (true) {
        std::cout << "Waiting for connections...\n";

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        handle_connection(&server_fd);
        // pthread_t thread_serve;
        // pthread_create(&thread_serve, NULL, handle_connection, &new_socket);
        // pthread_detach(thread_serve);

        // std::ifstream file("different_thread.txt");
        // std::string word;

        // while (file >> word) {
        //     std::cout << word << std::endl;
        // }
        // file.close();
    }

    return 0;
}