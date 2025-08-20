#ifndef CAPTURE_SERVER_HPP
#define CAPTURE_SERVER_HPP

#include <cstdint>
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

enum class req_type { FILE, DIRECTORY, COMMAND, PHP, ERROR, UNKNOWN };

inline std::string type_string(req_type type) {
  switch (type) {
  case req_type::FILE:
    return "FILE";
  case req_type::DIRECTORY:
    return "DIRECTORY";
  case req_type::COMMAND:
    return "COMMAND";
  case req_type::PHP:
    return "PHP";
  case req_type::ERROR:
    return "ERROR";
  case req_type::UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

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
  int thread_id;
  std::string path;
  std::string command;
  std::string args;

  std::string print() const {
    std::string type_str = type_string(type);
    return "Method: " + method + " | Ver: " + version +
           " | Raw: " + raw_path.substr(0, raw_path.find("HTTP")) +
           " | Type: " + type_str + " | Path: " + path + " | Cmd: " + command +
           " | Args: '" + args + "'\n";
  }
};

enum class log_level { TRACE, INFO, ERROR };

inline std::string log_string(log_level level) {

  switch (level) {
  case log_level::INFO:
    return "INFO";
  case log_level::TRACE:
    return "TRACE";
  case log_level::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

class ConnectionContext {
private:
  char request_buffer[BUFFER_SIZE];
  char response_buffer[BUFFER_SIZE];
  int socket_fd;
  RequestInfo request_info;
  bool socket_closed;
  uint64_t request_id;
  bool suppress_logging_for_request;
  int thread_id;

public:
  ConnectionContext(int thread_id); // Default constructor for pre-allocation
  ~ConnectionContext();

  void reset(int fd); // Reset context for new connection
  void cleanup();     // Clean up after request

  bool readRequest();
  bool parseRequest();
  bool sendResponse(const std::string &status, const std::string &content_type,
                    const std::string &body);
  bool sendResponseHeader(const std::string &status,
                          const std::string &content_type,
                          size_t content_length = 0);
  bool sendData(const char *data, size_t length);
  bool sendFile(const std::string &filepath);

  void handleRequest();

  RequestInfo &getRequestInfo() { return request_info; }
  int getSocketFd() const { return socket_fd; }
  char *getResponseBuffer() { return response_buffer; }
  int getThreadId() const { return thread_id; }
  void log(log_level level, const std::string &message,
           req_type type = req_type::UNKNOWN) const;

private:
  bool handleCommandRequest();
  bool handleFileRequest();
  bool handlePhpRequest(const std::string &php_path,
                        const std::string &args = "");
  bool executePHP(const std::string &php_command);
  void sendErrorResponse(const std::string &message);
  inline std::string determineContentType(const std::string &filepath);
};

void spawn_and_capture(char *argv[], std::stringstream &output);
void scan_directory(const std::string &directory,
                    std::vector<std::string> &filenames);
inline std::string log_level_to_string(log_level level);
#endif
