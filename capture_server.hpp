#ifndef CAPTURE_SERVER_HPP
#define CAPTURE_SERVER_HPP

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <spawn.h>
#include <sstream>
#include <string.h>
#include <sys/_pthread/_pthread_types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <cstdint>

enum class req_type {
    FILE,
    DIRECTORY,
    COMMAND,
    PHP,
    ERROR
};

#define NUM_THREADS 4
#define BUFFER_SIZE 4096
#define npos std::string::npos

struct RequestInfo {
    // HTTP request info
    std::string method;
    std::string version;
    std::string raw_path;
    // Request type
    req_type type;
    // Request internal details
    std::string path;
    std::string command;
    std::string args;

    std::string print() const {
    std::string type_str;
    switch (type) {
        case req_type::COMMAND: type_str = "COMMAND"; break;
        case req_type::FILE: type_str = "FILE"; break;
        case req_type::PHP: type_str = "PHP"; break;
        case req_type::DIRECTORY: type_str = "DIRECTORY"; break;
        case req_type::ERROR: type_str = "ERROR"; break;
        default: type_str = "UNKNOWN"; break;
    }
    return "Method: " + method +
           " | Ver: " + version +
           " | Raw: " + raw_path.substr(0, raw_path.find("HTTP")) +
           " | Type: " + type_str +
           " | Path: " + path +
           " | Cmd: " + command +
           " | Args: '" + args + "'\n";
}
};


class ConnectionContext {
private:
    char request_buffer[BUFFER_SIZE];
    char response_buffer[BUFFER_SIZE];
    int socket_fd;
    RequestInfo request_info;
    bool socket_closed;
    uint64_t request_id;
    bool suppress_logging_for_request;
    
public:
    ConnectionContext();  // Default constructor for pre-allocation
    ~ConnectionContext();
    
    void reset(int fd);  // Reset context for new connection
    void cleanup();      // Clean up after request
    
    bool readRequest();
    bool parseRequest();
    bool sendResponse(const std::string& status, const std::string& content_type, const std::string& body);
    bool sendResponseHeader(const std::string& status, const std::string& content_type, size_t content_length = 0);
    bool sendData(const char* data, size_t length);
    bool sendFile(const std::string& filepath);
    
    void handleRequest(int thread_id);
    
    RequestInfo& getRequestInfo() { return request_info; }
    int getSocketFd() const { return socket_fd; }
    char* getResponseBuffer() { return response_buffer; }
    
private:
    bool handleCommandRequest();
    bool handleFileRequest();
    bool handlePhpRequest(const std::string& php_path, const std::string& args = "");
    bool executePHP(const std::string& php_command);
    void sendErrorResponse(const std::string& message);
    inline std::string determineContentType(const std::string& filepath);

    enum log_level {
        INFO,
        TRACE,
        ERROR
    };
    void log(int thread_id, log_level level, const std::string &message) const;
    void log(log_level level, const std::string &message) const;
    // Logging helpers (keep messages concise)
    inline void logInfo(const std::string &message) const;
    inline void logTrace(const std::string &message) const;
    inline void logError(const std::string &message) const;
};

void spawn_and_capture(char *argv[], std::stringstream &output);
void scan_directory(const std::string &directory, std::vector<std::string> &filenames);

#endif
