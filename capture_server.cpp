#include <fstream>
#include <iostream>
#include <queue>
#include <regex>
#include "capture_server.hpp"

extern char **environ;

// Structure to pass both pool and context to each thread
struct ThreadData {
    class ThreadPool* pool;
    ConnectionContext* context;
};

// Simple thread pool class
class ThreadPool {
public:
  ThreadPool(size_t num_threads) { start(num_threads); }

  ~ThreadPool() { stop(); }

  void enqueue(int socket_fd) {
    pthread_mutex_lock(&tasks_mutex);
    tasks.push(socket_fd);
    pthread_cond_signal(&tasks_cond);
    pthread_mutex_unlock(&tasks_mutex);
  }

private:
  std::vector<pthread_t> threads;
  std::vector<ThreadData> thread_data;      // Data passed to each thread
  pthread_cond_t tasks_cond;
  pthread_mutex_t tasks_mutex;
  bool stopping;
  std::queue<int> tasks;

  void start(size_t num_threads) {
    pthread_cond_init(&tasks_cond, NULL);
    pthread_mutex_init(&tasks_mutex, NULL);
    stopping = false;

    thread_data.resize(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
      thread_data[i].pool = this;
      thread_data[i].context = new ConnectionContext();
      
      pthread_t thread;
      pthread_create(&thread, NULL, worker_thread, &thread_data[i]);
      threads.push_back(thread);
    }
  }

  void stop() {
    pthread_mutex_lock(&tasks_mutex);
    stopping = true;
    pthread_mutex_unlock(&tasks_mutex);
    pthread_cond_broadcast(&tasks_cond);

    for (pthread_t &thread : threads) {
      pthread_join(thread, NULL);
    }

    pthread_cond_destroy(&tasks_cond);
    pthread_mutex_destroy(&tasks_mutex);
  }

  static void *worker_thread(void *arg) {
    ThreadData *data = static_cast<ThreadData *>(arg);
    ThreadPool *pool = data->pool;
    ConnectionContext *ctx = data->context;

    while (true) {
      int socket_fd;

      pthread_mutex_lock(&pool->tasks_mutex);

      while (pool->tasks.empty() && !pool->stopping) {
        pthread_cond_wait(&pool->tasks_cond, &pool->tasks_mutex);
      }

      if (pool->stopping && pool->tasks.empty()) {
        pthread_mutex_unlock(&pool->tasks_mutex);
        break;
      }

      socket_fd = pool->tasks.front();
      pool->tasks.pop();

      pthread_mutex_unlock(&pool->tasks_mutex);

      // Use the pre-allocated context for this thread
      ctx->reset(socket_fd);
      ctx->handleRequest();
    }

    return NULL;
  }
};

// ConnectionContext Implementation  
ConnectionContext::ConnectionContext() : socket_fd(-1), socket_closed(true) {
    memset(request_buffer, 0, BUFFER_SIZE); 
    memset(response_buffer, 0, BUFFER_SIZE);
    memset((void*)&request_info, 0, sizeof(request_info));
    reset(-1);  // Initialize with invalid socket
}

ConnectionContext::~ConnectionContext() {
    cleanup();
}

void ConnectionContext::reset(int fd) {
    socket_fd = fd;
    socket_closed = false;
    request_buffer[0] = '\0';
    response_buffer[0] = '\0';
    request_info.type = req_type::ERROR;
    request_info.path = "";
    request_info.command = "";
    request_info.args = "";
    request_info.method = "";
    request_info.version = "";
    request_info.raw_path = "";
}

void ConnectionContext::cleanup() {
    if (!socket_closed && socket_fd >= 0) {
        close(socket_fd);
    }
    socket_fd = -1;
    socket_closed = true;
    memset(request_buffer, 0, BUFFER_SIZE);
    memset(response_buffer, 0, BUFFER_SIZE);
}

std::string RequestInfo::print() const {
    std::string type_str;
    switch (type) {
        case req_type::COMMAND: type_str = "COMMAND"; break;
        case req_type::FILE: type_str = "FILE"; break;
        case req_type::PHP: type_str = "PHP"; break;
        case req_type::DIRECTORY: type_str = "DIRECTORY"; break;
        case req_type::ERROR: type_str = "ERROR"; break;
        default: type_str = "UNKNOWN"; break;
    }
    return "\nMethod: " + method + "\n" +
           "Version: " + version + "\n" +
           "Raw Path: " + raw_path.substr(0, raw_path.find("HTTP")) + "\n\n" + 
           "Type: " + type_str + "\n" + 
           "Path: " + path + "\n" +
           "Command: " + command + "\n" +
           "Args: " + args + "\n\n";
}

bool ConnectionContext::readRequest() {
    std::string request;
    ssize_t bytes_read;
    
    do {
        bytes_read = read(socket_fd, request_buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            request_buffer[bytes_read] = '\0';
            request.append(request_buffer);
            
            if (request.find("\r\n\r\n") != npos) {
                break;
            }
        }
        request_buffer[0] = '\0';
    } while (bytes_read > 0);
    
    if (bytes_read == -1) {
        std::cerr << "Error reading request data." << std::endl;
        return false;
    }
    
    // Store the raw request for parsing
    request_info.raw_path = request;
    return true;
}

bool ConnectionContext::parseRequest() {
    std::istringstream request_stream(request_info.raw_path);
    std::string request_line;
    
    std::getline(request_stream, request_line);
    std::istringstream request_line_stream(request_line);
    request_line_stream >> request_info.method >> request_info.path >> request_info.version;
    
    // Remove dangerous path traversal attempts
    std::regex dotdot_regex("\\.\\./");
    request_info.path = std::regex_replace(request_info.path, dotdot_regex, "");
    
    std::cout << "Received HTTP request: " << request_info.method << " " << request_info.path << std::endl;
    
    // Remove leading slash
    if (!request_info.path.empty() && request_info.path[0] == '/') {
        request_info.path = request_info.path.substr(1);
    }
      // Index request, render index.php
    if (request_info.path.empty() || request_info.path == "/") {
        request_info.path = "./serving_files/index.php";
        request_info.type = req_type::PHP;
        
    } else if (request_info.path.find("?file=") != npos && request_info.path.find("&arguments=") != npos) {
        // Command execution request from index.php
        size_t file_index = request_info.path.find("=");
        size_t file_end_index = request_info.path.find("&");
        size_t arg_index = request_info.path.find_last_of('=');
        
        request_info.command = request_info.path.substr(file_index + 1, file_end_index - file_index - 1);
        request_info.args = request_info.path.substr(arg_index + 1);
        request_info.type = req_type::COMMAND;
        
    } else if (request_info.path.find("?raw_file=") != npos) {
        // Raw file viewing request from browse_files.php
        size_t file_index = request_info.path.find("raw_file=");
        std::string file_path = request_info.path.substr(file_index + 9); // 9 = length of "raw_file="
        
        // Remove any trailing parameters or HTTP version
        size_t end_pos = file_path.find_first_of("& ");
        if (end_pos != npos) {
            file_path = file_path.substr(0, end_pos);
        }
        
        // For now, simple handling of %2F -> /
        size_t pos = 0;
        while ((pos = file_path.find("%2F", pos)) != npos) {
            file_path.replace(pos, 3, "/");
            pos += 1;
        }
        
        request_info.path = "./serving_files/" + file_path;
        request_info.type = req_type::FILE;
        request_info.args = "raw";
        
    } else if (request_info.path.find("browse_files.php") != npos) {
        // Preserve original path with potential query string to extract dir parameter
        std::string original_path = request_info.path;
        request_info.path = "./serving_files/browse_files.php";

        // Extract dir query parameter if present
        std::string dir_value;
        size_t qpos = original_path.find("?dir=");
        if (qpos != npos) {
            dir_value = original_path.substr(qpos + 5);
            size_t end_pos = dir_value.find_first_of("& ");
            if (end_pos != npos) {
                dir_value = dir_value.substr(0, end_pos);
            }

            // Decode minimal encoding for forward slashes
            size_t pos = 0;
            while ((pos = dir_value.find("%2F", pos)) != npos) {
                dir_value.replace(pos, 3, "/");
                pos += 1;
            }

            // Pass as key=value so PHP CLI can parse into $_GET via argv
            request_info.args = "dir=" + dir_value;
            request_info.type = req_type::DIRECTORY;
        } else {
            request_info.args = "";
            request_info.type = req_type::PHP;
        }
    } else { // Regular file request
        // Check if it's a PHP file
        if (request_info.path.find(".php") != npos) {
            request_info.type = req_type::PHP;
            // Extract query string if present and strip from path
            size_t qmark_index = request_info.path.find("?");
            if (qmark_index != npos) {
                request_info.args = request_info.path.substr(qmark_index + 1);
                size_t end_pos = request_info.args.find_first_of(" ");
                if (end_pos != npos) {
                    request_info.args = request_info.args.substr(0, end_pos);
                }
                request_info.path = request_info.path.substr(0, qmark_index);
            }
            // Add serving_files path if not already present
            if (request_info.path.find("serving_files") == npos) {
                request_info.path = "./serving_files/" + request_info.path;
            }
        } else {
            // Assume it's a regular file
            request_info.type = req_type::FILE;
            // Add serving_files path for files if not already present
            request_info.path = "./serving_files/" + request_info.path;
        }
    }
    std::cout << request_info.print() << std::endl;
    return true;
}

void ConnectionContext::handleRequest() {
    if (!readRequest()) {
        return;
    }
    
    if (!parseRequest()) {
        sendErrorResponse("Failed to parse request");
        return;
    }
    
    bool success = false;
    
    switch (request_info.type) {
        case req_type::COMMAND:
            success = handleCommandRequest();
            break;
        case req_type::FILE:
            success = handleFileRequest();
            break;
        case req_type::DIRECTORY:
        case req_type::PHP:
            success = handlePhpRequest(request_info.path, request_info.args);
            break;
        case req_type::ERROR:
        default:
            sendErrorResponse("Invalid request type");
            break;
    }
    if (success) {
        std::cout << "Request handled successfully\n";
    }
}

bool ConnectionContext::handleCommandRequest() {
    std::vector<std::string> filenames;
    scan_directory("./Executables", filenames);
    
    bool found_executable = false;
    std::string full_command = request_info.command;
    
    for (const std::string& file : filenames) {
        if (request_info.command == file) {
            full_command = "./Executables/" + request_info.command;
            found_executable = true;
            break;
        }
    }
    
    if (!found_executable) {
        sendErrorResponse("Executable not found: " + request_info.command);
        return false;
    }
    
    std::cout << "Executing command: " << full_command << " with args: " << request_info.args << std::endl;
    
    // Prepare argv for spawn
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(full_command.c_str()));
    
    // Parse arguments
    std::istringstream args_stream(request_info.args);
    std::string arg;
    std::vector<std::string> arg_storage;
    
    while (args_stream >> arg) {
        arg_storage.push_back(arg);
    }
    
    for (auto& stored_arg : arg_storage) {
        argv.push_back(const_cast<char*>(stored_arg.c_str()));
    }
    argv.push_back(NULL);
    
    // Capture command output
    std::stringstream output;
    spawn_and_capture(argv.data(), output);
    
    // Send response header
    sendResponseHeader("200 OK", "text/html");
    
    // Execute index.php with the output
    std::string php_command = "php ./serving_files/index.php '" + output.str() + "'";
    return executePHP(php_command);
}

bool ConnectionContext::handleFileRequest() {
    // Check if this is a raw file request (from browse_files.php)
    bool is_raw_request = (request_info.args == "raw");
    
    // If it's a PHP file and NOT a raw request, execute it
    if (!is_raw_request && request_info.path.find(".php") != npos) {
        return handlePhpRequest(request_info.path);
    }
    
    // Otherwise, serve the file content (including PHP files as raw text)
    FILE *file = fopen(request_info.path.c_str(), "rb");
    
    if (file == NULL) {
        sendErrorResponse("File not found: " + request_info.path);
        return false;
    }
    
    // Determine content type
    std::string content_type;
    if (is_raw_request && request_info.path.find(".php") != npos) {
        // Serve PHP files as plain text when requested raw
        content_type = "text/plain";
    } else {
        content_type = determineContentType(request_info.path);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Send header
    sendResponseHeader("200 OK", content_type, file_size);
    
    // Send file content
    while (!feof(file) && !ferror(file)) {
        size_t bytes_read = fread(response_buffer, 1, BUFFER_SIZE, file);
        
        if (bytes_read > 0) {
            if (!sendData(response_buffer, bytes_read)) {
                return false;
            }
        }
    }
    
    return !ferror(file);
}

bool ConnectionContext::handlePhpRequest(const std::string& php_path, const std::string& args) {
    // Always send HTML header for PHP execution
    sendResponseHeader("200 OK", "text/html");
    
    // Build PHP command
    std::string php_command = "php " + php_path;
    
    if (!args.empty()) {
        php_command += " '" + args + "'";
    }
    
    return executePHP(php_command);
}

bool ConnectionContext::executePHP(const std::string& php_command) {
    // Do not blindly append raw query strings. Any needed args should be
    // provided by callers (e.g., handlePhpRequest) based on parsed request_info.
    std::string final_command = php_command;

    std::cout << "Executing PHP: " << final_command << std::endl;
    
    FILE* fp = popen(final_command.c_str(), "r");
    if (fp == NULL) {
        std::cerr << "Error executing PHP command" << std::endl;
        return false;
    }
    
    while (fgets(response_buffer, BUFFER_SIZE, fp) != NULL) {
        size_t len = strlen(response_buffer);
        if (!sendData(response_buffer, len)) {
            pclose(fp);
            return false;
        }
        response_buffer[0] = '\0';
    }
    
    return pclose(fp) != -1;
}

bool ConnectionContext::sendResponse(const std::string& status, const std::string& content_type, const std::string& body) {
    std::string response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Connection: close\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "\r\n";
    response += body;
    
    return write(socket_fd, response.c_str(), response.length()) != -1;
}

bool ConnectionContext::sendResponseHeader(const std::string& status, const std::string& content_type, size_t content_length) {
    std::string header = "HTTP/1.1 " + status + "\r\n";
    header += "Content-Type: " + content_type + "\r\n";
    header += "Connection: close\r\n";
    
    if (content_length > 0) {
        header += "Content-Length: " + std::to_string(content_length) + "\r\n";
    }
    
    header += "\r\n";
    
    return write(socket_fd, header.c_str(), header.length()) != -1;
}

bool ConnectionContext::sendData(const char* data, size_t length) {
    if (socket_closed) return false;
    
    ssize_t written = write(socket_fd, data, length);
    if (written == -1) {
        socket_closed = true;
        return false;
    }
    
    return true;
}

void ConnectionContext::sendErrorResponse(const std::string& message) {
    std::string html = "<html><body><h1>404 Not Found</h1>";
    html += "<p>" + message + "</p>";
    html += "<a href='/'>Back to home</a></body></html>";
    
    sendResponse("404 Not Found", "text/html", html);
}

inline std::string ConnectionContext::determineContentType(const std::string& filepath) {
    size_t extension_index = filepath.find_last_of('.');
    
    if (extension_index == npos) {
        return "text/plain";
    }
    
    std::string extension = filepath.substr(extension_index + 1);
    
    if (extension == "html") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    
    return "text/plain";
}

// Global helper functions
void spawn_and_capture(char *argv[], std::stringstream &output) {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        output << "Error creating pipe\n";
        return;
    }
    
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);
    posix_spawn_file_actions_addclose(&actions, pipefd[1]);
    
    char *program = argv[0];
    
    if (posix_spawn(&pid, program, &actions, NULL, argv, environ) != 0) {
        perror("posix_spawn");
        output << "Error spawning process\n";
        posix_spawn_file_actions_destroy(&actions);
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    
    posix_spawn_file_actions_destroy(&actions);
    
    printf("Process %s started with PID: %d => child process PID: %d\n\n", program, getpid(), pid);
    
    close(pipefd[1]);
    
    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        output << buffer;
    }
    
    close(pipefd[0]);
    
    int status;
    waitpid(pid, &status, 0);
}

void scan_directory(const std::string &directory, std::vector<std::string> &filenames) {
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr) {
        std::cerr << "Failed to open directory: " << directory << std::endl;
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            filenames.push_back(entry->d_name);
        }
    }
    closedir(dir);
}


// Main function
int main() {
    int server_fd;
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
    
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Listening on port 8080...\n";
    
    ThreadPool pool(NUM_THREADS);
    
    while (true) {
        std::cout << "Waiting for connections...\n";
        
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }
        
        pool.enqueue(new_socket);
    }
    
    return 0;
}