#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <cstring>
#include <sstream>
#include <regex>
#include <pthread.h>

void send_file(int socket_fd, const std::string& path, const std::string& content_type);
void* handle_connection(void* socket_fd_ptr);

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening on port 8080...\n";

    int connection_count = 0;

    while (true) {
        std::cout << "Waiting for connections...\n";

        // Accept a new connection
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        connection_count++;
        std::cout << "Connection " << connection_count << " accepted\n";

        // Create a new thread to handle the connection
        pthread_t thread;
        int* socket_fd_ptr = new int(new_socket);
        pthread_create(&thread, NULL, handle_connection, (void*)socket_fd_ptr);
        pthread_detach(thread);

        // Demonstrate that the main thread continues execution while the connection is being handled
        std::cout << "Main thread continues execution...\n";
    }

    return 0;
}

void* handle_connection(void* socket_fd_ptr) {
    int socket_fd = *(int*)socket_fd_ptr;
    delete (int*)socket_fd_ptr;

    char buffer[4096];
    ssize_t bytes_read;

    std::string request;
    do {
        // Read the request data
        bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            request.append(buffer);

            // Check if the end of headers is reached
            if (request.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
    } while (bytes_read > 0 && bytes_read < (sizeof(buffer) - 1));

    if (bytes_read == -1) {
        std::cerr << "Error reading request data." << std::endl;
        close(socket_fd);
        return NULL;
    }

    std::cout << "Received HTTP request:\n" << request << std::endl;

    // Parse the request line
    std::istringstream request_stream(request);
    std::string request_line;

    std::getline(request_stream, request_line);

    std::istringstream request_line_stream(request_line);
    std::string method, path, version;
    request_line_stream >> method >> path >> version;

    // Remove query parameters and "../" from the path
    std::regex query_regex("\\?.*");
    std::regex dotdot_regex("\\.\\./");

    path = std::regex_replace(path, query_regex, "");
    path = std::regex_replace(path, dotdot_regex, "");

    // Remove leading slash from the path
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }

    // Set default path if empty or "/"
    if (path.empty() || path == "/") {
        path = "../serving_files/index.html";
    }
    else {
        path = "../serving_files/" + path;
    }
    std::cout<<"Path Retrieved: "<< path << std::endl;
    // Get the file extension
    std::string extension;
    size_t extension_index = path.find_last_of('.');

    if (extension_index != std::string::npos) {
        extension = path.substr(extension_index + 1);
    }

    // Set the content type based on the file extension
    std::string content_type;

    if (extension == "html") {
        content_type = "text/html";
    } else if (extension == "css") {
        content_type = "text/css";
    } else if (extension == "js") {
        content_type = "application/javascript";
    } else if (extension == "png") {
        content_type = "image/png";
    } else if (extension == "jpg" || extension == "jpeg") {
        content_type = "image/jpeg";
    } else if (extension == "gif") {
        content_type = "image/gif";
    } else {
        content_type = "text/plain";
    }

    // Send the file
    send_file(socket_fd, path, content_type);

    return NULL;
}

void send_file(int socket_fd, const std::string& path, const std::string& content_type) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        // File not found, send 404 response
        std::string error_message = "File not found";
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: " + std::to_string(error_message.length()) + "\r\n\r\n" + error_message;
        write(socket_fd, response.data(), response.size());
    } else {
        // Get the file size
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Prepare the HTTP response header
        std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nConnection: close\r\nContent-Length: " + std::to_string(file_size) + "\r\n\r\n";
        write(socket_fd, header.data(), header.size());

        // Send the file content
        char buffer[4096];
        while (!file.eof() && !file.bad()) {
            file.read(buffer, sizeof(buffer));
            size_t bytes_read = file.gcount();

            if (bytes_read > 0) {
                write(socket_fd, buffer, bytes_read);
            }
        }
        if(file.bad()){
            std::cerr << "Error reading file.\n";
            close(socket_fd);
        }
    }

    std::cout << "Sent HTTP response with file.\n";
    close(socket_fd);
}
