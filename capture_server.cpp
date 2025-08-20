#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include <regex>
#include <spawn.h>
#include <sstream>
#include <string.h>
#include <sys/_pthread/_pthread_types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void send_file(int socket_fd, const std::string &path);
void handle_connection(int socket_fd);
void spawn_and_capture(char *argv[], bool hasProgram,
                       std::stringstream &output);
void scan_directory(const std::string &directory,
                    std::vector<std::string> &filenames);

extern char **environ;

// Simple thread pool class
class ThreadPool {
public:
  ThreadPool(size_t num_threads) { start(num_threads); }

  ~ThreadPool() { stop(); }

  void enqueue(int socket_fd) {
    // Add the socket file descriptor to the tasks queue
    tasks.push(socket_fd);
    // Signal a waiting thread that a new task is available
    pthread_cond_signal(&tasks_cond);
  }

private:
  std::vector<pthread_t> threads;
  pthread_cond_t tasks_cond;
  pthread_mutex_t tasks_mutex;
  bool stopping;
  std::queue<int> tasks;

  void start(size_t num_threads) {
    // Initialize the condition variable and mutex
    pthread_cond_init(&tasks_cond, NULL);
    pthread_mutex_init(&tasks_mutex, NULL);
    stopping = false;

    // Create the worker threads
    for (size_t i = 0; i < num_threads; ++i) {
      pthread_t thread;
      pthread_create(&thread, NULL, worker_thread, this);
      threads.push_back(thread);
    }
  }

  void stop() {
    // Set the stopping flag to true
    stopping = true;
    // Signal all threads to wake up
    pthread_cond_broadcast(&tasks_cond);

    // Wait for all threads to finish
    for (_opaque_pthread_t *&thread : threads) {
      pthread_join(thread, NULL);
    }

    // Clean up the condition variable and mutex
    pthread_cond_destroy(&tasks_cond);
    pthread_mutex_destroy(&tasks_mutex);
  }

  static void *worker_thread(void *arg) {
    ThreadPool *pool = static_cast<ThreadPool *>(arg);

    while (true) {
      int socket_fd;

      // Acquire the mutex lock
      pthread_mutex_lock(&pool->tasks_mutex);

      // Wait until there is a task in the queue or the pool is stopping
      while (pool->tasks.empty() && !pool->stopping) {
        pthread_cond_wait(&pool->tasks_cond, &pool->tasks_mutex);
      }

      // If the pool is stopping and there are no more tasks, exit the thread
      if (pool->stopping && pool->tasks.empty()) {
        pthread_mutex_unlock(&pool->tasks_mutex);
        break;
      }

      // Get the next task from the queue
      socket_fd = pool->tasks.front();
      pool->tasks.pop();

      // Release the mutex lock
      pthread_mutex_unlock(&pool->tasks_mutex);

      // Handle the connection
      handle_connection(socket_fd);
    }

    return NULL;
  }
};

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

  // Set socket options to allow reuse of address and port
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

  // Create a thread pool with 4 threads
  // This can be easily modified to read a configuration file or command line
  // arguments
  ThreadPool pool(4);

  while (true) {
    std::cout << "Waiting for connections...\n";

    // Accept a new connection
    int new_socket =
        accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (new_socket < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    // Enqueue the new connection to the thread pool
    pool.enqueue(new_socket);
  }

  return 0;
}

void handle_connection(int socket_fd) {
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
    buffer[0] = '\0';
  } while (bytes_read > 0);

  if (bytes_read == -1) {
    std::cerr << "Error reading request data." << std::endl;
    close(socket_fd);
    return;
  }

  // Parse the request line
  std::istringstream request_stream(request);
  std::string request_line;

  std::getline(request_stream, request_line);

  std::istringstream request_line_stream(request_line);
  std::string method, path, version;
  request_line_stream >> method >> path >> version;

  // Remove query parameters and "../" from the path
  std::regex dotdot_regex("\\.\\./");

  path = std::regex_replace(path, dotdot_regex, "");

  std::cout << "Received HTTP request for:\n" << path << std::endl;

  // Remove leading slash from the path
  if (!path.empty() && path[0] == '/') {
    path = path.substr(1);
  }
  std::string command;
  // Set default path if empty or "/"
  if (path.empty() || path == "/") {
    path = "./serving_files/index.php";

    // If the path was a request to run an executable
  } else if (path.find("?file=") != std::string::npos &&
             path.find("&arguments=") != std::string::npos) {

    size_t file_index = path.find_first_of("=");
    size_t arg_index = path.find_first_of("&");
    std::string command_path =
        path.substr(file_index + 1, arg_index - file_index - 1);
    std::string arguments = path.substr(arg_index + 11);
    command = command_path + " " + arguments;
    path = "/command/" + command;

  } else if (path.find("?dir=") != std::string::npos) {
    if (path.find("&file=") != std::string::npos) {
      size_t dir_index = path.find_first_of("=");
      size_t dir_end_index = path.find_first_of("&");
      size_t file_index = path.find_last_of("=");
      std::string dir =
          path.substr(dir_index + 1, dir_end_index - dir_index - 1);
      std::string file = path.substr(file_index + 1);
      path = "./serving_files/" + file;

    } else {
      size_t dir_index = path.find_first_of("=");
      std::string dir = path.substr(dir_index + 5);
      path = " -lh ./serving_files/" + dir;
      command = "ls";
      path = "/command/" + command + path;
    }
  } else {
    std::string file_path = path;
    file_path = "./serving_files/" + file_path;

    path = file_path;
  }

  std::cout << "Requested path: " << path << std::endl;

  // Send the file or execute the command
  send_file(socket_fd, path);
}

void send_file(int socket_fd, const std::string &path) {
  char buffer[4096];
  size_t bytes_read;

  if (path.find("/command/") != std::string::npos) {

    std::vector<std::string> filenames;
    scan_directory("./Executables", filenames);

    std::string command_string = path.substr(9);

    std::string command =
        command_string.substr(0, command_string.find_first_of(" "));
    std::string args_string =
        command_string.substr(command_string.find_first_of(" ") + 1);

    bool hasProgram =
        (command.find_first_of(
             "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") !=
         std::string::npos);

    for (std::string file : filenames) {
      if (command.find(file) != std::string::npos) {
        command = "./Executables/" + command;
        break;
      }
    }
    if (command.find("./Executables/") == std::string::npos) {
      command = "/bin/" + command;
    }

    std::cout << "Command: " << command;
    std::cout << " Arguments: " << args_string << std::endl;

    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(command.c_str()));

    std::istringstream args_stream(args_string);
    std::string arg;

    while (args_stream >> arg) {
      argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(NULL);

    std::stringstream output;
    spawn_and_capture(argv.data(), hasProgram, output);

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: "
                           "text/html\r\nConnection: close\r\n\r\n";
    write(socket_fd, response.data(), response.size());

    std::string index_php =
        "php ./serving_files/index.php '" + output.str() + "'";

    std::cout << "\nExecuting command: " << command << " from PID: " << getpid()
              << std::endl;

    FILE *fp = popen(index_php.c_str(), "r");
    if (fp == NULL) {
      std::cout << "Error executing command.\n";
      close(socket_fd);
      return;
    }

    for (int i; i < 4095; i++) {
      if (buffer[i] == '+')
        buffer[i] = ' ';
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
      write(socket_fd, buffer, strlen(buffer));
      buffer[0] = '\0';
    }

    if (pclose(fp) == -1) {
      std::cout << "Error closing pipe.\n";
      close(socket_fd);
      return;
    }
  } else {

    std::ifstream output(path, std::ios::binary);

    if (!output) {
      // File not found, send 404 response
      std::string error_message = "File or command not found";
      std::string response =
          "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: "
          "close\r\nContent-Length: " +
          std::to_string(error_message.length()) + "\r\n\r\n" + error_message;
      write(socket_fd, response.data(), response.size());
    } else {

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

      if (extension == "php" || path == "./serving_files/index.php" ||
          path == "./serving_files/browse_files.php") {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: "
                               "text/html\r\nConnection: close\r\n\r\n";
        write(socket_fd, response.data(), response.size());

        std::string command = "php " + path;

        std::cout << "\nExecuting command: " << command
                  << " from PID: " << getpid() << std::endl;

        FILE *fp = popen(command.c_str(), "r");
        if (fp == NULL) {
          std::cout << "Error executing command.\n";
          close(socket_fd);
          return;
        }

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
          write(socket_fd, buffer, strlen(buffer));
          buffer[0] = '\0';
        }

        if (pclose(fp) == -1) {
          std::cout << "Error closing pipe.\n";
          close(socket_fd);
          return;
        }
        return;
      }

      // Get the file size
      output.seekg(0, std::ios::end);
      size_t file_size = output.tellg();
      output.seekg(0, std::ios::beg);

      // Prepare the HTTP response header
      std::string header = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type +
                           "\r\nConnection: close\r\nContent-Length: " +
                           std::to_string(file_size) + "\r\n\r\n";
      write(socket_fd, header.data(), header.size());

      // Send the file content

      while (!output.eof() && !output.bad()) {
        output.read(buffer, sizeof(buffer));
        size_t bytes_read = output.gcount();

        if (bytes_read > 0) {
          write(socket_fd, buffer, bytes_read);
          buffer[0] = '\0';
        }
      }
      if (output.bad()) {
        std::cerr << "Error reading file.\n";
        close(socket_fd);
      }
    }
  }

  std::cout << "Sent HTTP response.\n";
  close(socket_fd);
}

void spawn_and_capture(char *argv[], bool hasProgram,
                       std::stringstream &output) {

  int pipefd[2];
  pid_t pid;

  char buffer[4096];
  ssize_t bytes_read;

  // Create a pipe
  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  // Set up file actions for posix_spawn
  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, pipefd[0]);
  posix_spawn_file_actions_addclose(&actions, pipefd[1]);

  if (!hasProgram) {
    output << "No program specified\n";
    return;
  }

  char *program = argv[0];

  // Spawn the child process
  if (posix_spawn(&pid, program, &actions, NULL, argv, environ) != 0) {
    perror("posix_spawn");
    exit(EXIT_FAILURE);
  }

  if (strcmp(program, "php") == 0) {
    output << "PHP script executed successfully\n";
    // Close the write end of the pipe in the parent process
    close(pipefd[1]);
  } else {
    printf("Process %s started with PID: %d => child process PID: %d\n\n",
           program, getpid(), pid);
    // Close the write end of the pipe in the parent process
    close(pipefd[1]);
    // Read the output from the child process and write it to the stringstream
    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
      output.write(buffer, bytes_read);
      buffer[0] = '\0';
    }
  }

  // Close the read end of the pipe
  close(pipefd[0]);

  // Wait for the child process to finish
  int status;
  waitpid(pid, &status, 0);

  return;
}

void scan_directory(const std::string &directory,
                    std::vector<std::string> &filenames) {

  DIR *dir = opendir(directory.c_str());
  if (dir == nullptr) {
    std::cerr << "Failed to open directory: " << directory << std::endl;
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_REG) { // Only consider regular files
      filenames.push_back(entry->d_name);
    }
  }
  closedir(dir);
}
