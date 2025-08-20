#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <cstring> 
#include <sstream>
#include <regex>

void send_file(int socket_fd, const std::string& path, const std::string& content_type);
void handle_connection(int socket_fd);


//The main function is nearly unchanged from the web server assignment.
//The difference are mainly in the handle_connection and the new send_file function.
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // There are a lot of Unix related specifics that I am not familiar with
    // I used the following code as a reference for the socket setup
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

        // System is blocking until a connection is made
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //When a new connection is made, handle the connection
        handle_connection(new_socket);
    }

    return 0;
}


void handle_connection(int socket_fd) {
    char buffer[4096]; // Buffer to store request data
    ssize_t bytes_read;

    // Read the request data until there's none left or buffer is full
    std::string request;
    do {
        bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate what we have read and add to the request string
            request.append(buffer);

            // Check if we've received the end of the headers
            if (request.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
    } while (bytes_read > 0 && bytes_read < (sizeof(buffer) - 1));

    // Check for read error
    if (bytes_read == -1) {
        std::cerr << "Error reading request data." << std::endl;
        close(socket_fd);
        return;
    }

    std::cout << "Received HTTP request:\n" << request << std::endl;

    // Parse the request line
    std::istringstream request_stream(request);
    std::string request_line;

    std::getline(request_stream, request_line);

    std::istringstream request_line_stream(request_line);
    std::string method, path, version;
    //Assuming the request is in the form of "GET /path/to/file.html HTTP/$VERSION"
    request_line_stream >> method >> path >> version;
    
    // Use regex to remove any query parameters from the path, as well as /../
    std::regex query_regex("\\?.*");
    std::regex dotdot_regex("\\.\\./");

    path = std::regex_replace(path, query_regex, "");
    path = std::regex_replace(path, dotdot_regex, "");

    // Remove leading slash to get file path
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }

    if (path.empty() || path == "/") {
        path = "./serving_files/index.html"; // Default file
    }
    else {
        path = "./serving_files/" + path;
    }

    // Find the extension of the file
    std::string extension;
    size_t extension_index = path.find_last_of('.');
    
    // Get the extension type after the last period
    if (extension_index != std::string::npos) {
        extension = path.substr(extension_index + 1);
    }

    std::string content_type;

    // Set the content type based on the file extension
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
}

void send_file(int socket_fd, const std::string& path, const std::string& content_type) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        // File not found or unable to open
        std::string error_message = "File not found";
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: " + std::to_string(error_message.length()) + "\r\n\r\n" + error_message;
        write(socket_fd, response.data(), response.size());
    } else {
        // Determine file size
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Prepare HTTP header
        std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nConnection: close\r\nContent-Length: " + std::to_string(file_size) + "\r\n\r\n";
        write(socket_fd, header.data(), header.size());

        // Send the file content in chunks
        char buffer[4096];
        while (!file.eof() && !file.bad()) {
            file.read(buffer, sizeof(buffer));
            size_t bytes_read = file.gcount(); // Number of bytes actually read

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
