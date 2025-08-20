# C++ Multi-threaded HTTP Server

A robust, multi-threaded HTTP server implementation written entirely in C++ that demonstrates  systems programming concepts including thread pooling, process forking with output piping, and dynamic file serving capabilities.

## Project Purpose

This project is a complete HTTP server implementation from scratch in C++, including:
- **Multi-threaded request handling** using POSIX threads and thread pools
- **Process forking and IPC** through `posix_spawn()` with pipe-based output capture
- **Dynamic content generation** via PHP script execution
- **Static file serving** with proper MIME type detection
- **Web-based interface** for command execution and file browsing

This server was developed as a systems programming exercise to demonstrate low-level networking, threading, and process management capabilities without relying on existing web server frameworks.

## Architecture Overview

### Core Components

1. **`capture_server.cpp`** - Main server implementation
   - Thread pool management with worker threads
   - Socket handling and HTTP request parsing
   - Request routing and response generation
   - Process spawning and output capture

2. **`capture_server.hpp`** - Server header definitions
   - `ConnectionContext` class for managing individual connections
   - Request type enumeration and structures
   - Thread pool configuration constants

3. **PHP Web Interface**
   - `index.php` - Interactive command executor interface
   - `browse_files.php` - Directory browser with file navigation
   - `code_view.php` - Syntax-highlighted code viewer for source files

## Key Features

### Multi-threaded Architecture
- **Thread Pool Pattern**: Pre-spawned worker threads (default: 4) handle incoming connections
- **Lock-free Request Queue**: Uses condition variables for efficient thread synchronization
- **Per-thread Context**: Each worker maintains its own `ConnectionContext` for isolation
- **Graceful Shutdown**: Signal handling for clean server termination

### Request Processing Pipeline
1. **Connection Acceptance**: Main thread accepts incoming connections
2. **Task Enqueueing**: New connections are added to the thread pool queue
3. **Worker Processing**: Available worker thread picks up the connection
4. **Request Parsing**: HTTP request is parsed to determine type (FILE, PHP, COMMAND, DIRECTORY)
5. **Response Generation**: Appropriate handler generates and sends the response
6. **Connection Cleanup**: Resources are properly released after response completion

### Process Spawning & Output Capture
- Uses `posix_spawn()` for secure process creation
- Implements pipe-based IPC for capturing subprocess output
- Supports execution of custom executables with arguments
- Real-time output streaming to web clients

### Dynamic Content Support
- **PHP Script Execution**: Runs PHP scripts via `popen()` with output buffering
- **Command Execution**: Web interface for running server-side executables
- **Query Parameter Parsing**: Supports GET parameters for dynamic content
- **Content-Length Headers**: Proper HTTP response headers for clean connection handling

### File Serving Capabilities
- **Static Files**: Serves HTML, CSS, JavaScript, images, and text files
- **Binary Files**: Handles PNG, JPEG, GIF, and WebAssembly files
- **Directory Browsing**: Interactive file browser with navigation
- **Code Viewing**: Syntax-highlighted source code display
- **Raw File Access**: Direct file download capability

## Technical Implementation Details

### Thread Pool Implementation
```cpp
class ThreadPool {
    std::vector<pthread_t> threads;
    std::queue<int> tasks;
    pthread_mutex_t tasks_mutex;
    pthread_cond_t tasks_cond;
    // Worker threads wait on condition variable
    // Main thread signals when new task arrives
};
```

### Connection Context Management
Each connection maintains its own state:
- Request/response buffers (4KB each)
- Socket file descriptor
- Request parsing information
- Logging control (suppresses logs for static assets)
- Unique request ID for tracing

### Security Features
- **Path Sanitization**: Removes `../` patterns to prevent directory traversal
- **Restricted Execution**: Executables limited to designated directories
- **Input Validation**: Query parameters are validated before processing
- **Signal Handling**: Proper handling of SIGPIPE to prevent crashes
- **Resource Limits**: Fixed buffer sizes prevent memory exhaustion

## Building and Running

### Prerequisites
- C++11 compatible compiler (g++ or clang++)
- POSIX threads support
- PHP CLI (for dynamic content)
- Standard UNIX development tools

### Compilation
```bash
# Standard compilation
g++ -std=c++11 -pthread capture_server.cpp -o capture_server

# With optimization
g++ -std=c++11 -O2 -pthread capture_server.cpp -o capture_server

# Debug build
g++ -std=c++11 -g -pthread capture_server.cpp -o capture_server
```

### Running the Server
```bash
./capture_server
```

Server output:
```
Listening on port 8080...
Waiting for connections...
[#0] T0 INFO: ok
[#1] T1 TRACE: recv GET /index.php
```

## Usage Examples

### Basic File Access
```bash
# Serve static file
curl http://localhost:8080/style/main.css

# Execute PHP script
curl http://localhost:8080/index.php

# Browse directories
curl "http://localhost:8080/browse_files.php?dir=style"
```

### Command Execution
```bash
# Run executable with arguments
curl "http://localhost:8080/?file=echo&arguments=Hello%20World"

# Run without arguments
curl "http://localhost:8080/?file=ascii_art&arguments="
```

### File Browser Navigation
1. Navigate to `http://localhost:8080/browse_files.php`
2. Click directories to browse
3. Click code files to view with syntax highlighting
4. Use "raw" links for direct file download

## Performance Characteristics

- **Concurrent Connections**: Handles 4 simultaneous requests (configurable)
- **Buffer Size**: 4KB for request/response buffers
- **Connection Model**: One thread per active connection from pool
- **Process Spawning**: Efficient fork/exec with output capture
- **Logging**: Configurable verbosity with request ID tracking

## Project Structure

```
http_server_final/
├── capture_server.cpp      # Main server implementation (~850 lines)
├── capture_server.hpp      # Header with class definitions
├── capture_server          # Compiled executable
├── Executables/            # Directory for custom executables
│   ├── alternating_case    # Example Executables
│   ├── ascii_art
│   ├── char_pyramid
│   ├── diamond_chars
│   ├── repeating_chars
│   ├── reverse_string
│   ├── rotating
│   └── spiral
└── serving_files/         # Web root directory
    ├── index.php          # Command executor interface
    ├── browse_files.php   # File browser
    ├── code_view.php      # Syntax-highlighted code viewer
    ├── style/             # CSS and fonts
    │   ├── main.css
    │   └── web/
    │       └── fonts/    # Hack monospace font
    └── [various test files and examples]
```

## Educational Value

This project demonstrates:
1. **Low-level Networking**: Raw socket programming with TCP/IP
2. **Concurrency Models**: Thread pools vs thread-per-connection
3. **IPC Mechanisms**: Pipes for parent-child communication
4. **HTTP Protocol**: Request parsing and response generation
5. **Process Management**: Fork/exec patterns in UNIX
6. **Resource Management**: Proper cleanup and error handling
7. **Security Considerations**: Input validation and sandboxing

## Potential Enhancements
- Configuration file support
- Dynamic thread pool sizing
- HTTPS support via OpenSSL
- Request body handling for POST
- File upload capabilities
- WebSocket implementation
- HTTP/2 protocol support
- Connection keep-alive
- Request rate limiting
- Access logging to files

## Testing

### Basic Functionality Tests
```bash
# Test static file serving
curl -I http://localhost:8080/index.html

# Test PHP execution
curl http://localhost:8080/index.php

# Test command execution
curl "http://localhost:8080/?file=echo&arguments=test"

# Test error handling
curl http://localhost:8080/nonexistent.file
```

### Load Testing
```bash
# Concurrent connections test
for i in {1..10}; do curl http://localhost:8080/ & done

# Sustained load test
while true; do curl -s http://localhost:8080/ > /dev/null; done
```

## Signal Handling

The server responds to the following signals:
- `SIGINT` (Ctrl+C): Graceful shutdown
- `SIGTERM`: Clean termination
- `SIGHUP`: Handled for daemon mode compatibility
- `SIGQUIT`: Emergency shutdown
- `SIGPIPE`: Ignored to prevent broken pipe crashes

## Logging

The server provides detailed logging with request IDs:
- **INFO**: Successful request completion
- **TRACE**: Detailed execution flow
- **ERROR**: Error conditions and failures

Log format: `[#RequestID] ThreadID LEVEL: message`

## License

This is an educational project developed for systems programming coursework.

## Author

Matthew Prock - Systems Programming Final Project 

## Acknowledgments

- POSIX threads documentation
- HTTP/1.1 RFC specifications
- C++ systems programming resources