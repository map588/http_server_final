# HTTP Server with Command Execution

A multi-threaded HTTP server written in C++ that serves static files, executes PHP scripts, and runs custom executables through a web interface.

## Features

- **Multi-threaded Architecture**: Uses a thread pool (4 worker threads) for concurrent request handling
- **Static File Serving**: Supports HTML, CSS, JavaScript, images (PNG, JPG, GIF), and plain text files
- **PHP Script Execution**: Dynamic execution of PHP scripts with output streaming
- **Command Execution**: Web-based interface to run executables with custom arguments
- **File Browser**: PHP-powered file browsing interface
- **Security Features**: Path sanitization to prevent directory traversal attacks

## Project Structure

```
http_server_final/
├── capture_server.cpp      # Main server implementation
├── capture_server          # Compiled server executable
├── Executables/           # Custom executables directory
│   ├── alternating_case
│   ├── ascii_art
│   ├── diamond
│   ├── echo
│   ├── pyramid
│   ├── repeating
│   ├── reverse
│   └── rotating
└── serving_files/         # Web root directory
    ├── index.php          # Main web interface for executable runner
    ├── browse_files.php   # File browser interface
    └── style/            # CSS and font resources
        ├── main.css
        └── web/
            └── fonts/    # Hack font files
```

## Building the Server

Compile the server using a C++ compiler with pthread support:

```bash
g++ -std=c++11 -pthread capture_server.cpp -o capture_server
```

## Running the Server

Start the server:

```bash
./capture_server
```

The server will:
- Listen on port 8080
- Display "Listening on port 8080..." when ready
- Show connection logs for each request

## Usage

### Accessing the Server

Open a web browser and navigate to:
- `http://localhost:8080` - Main interface (index.php)
- `http://localhost:8080/browse_files.php` - File browser

### URL Patterns

The server supports several URL patterns:

1. **Static Files**: `http://localhost:8080/filename.ext`
   - Serves files from the `serving_files` directory

2. **Execute Programs**: `http://localhost:8080/?file=program&arguments=args`
   - Executes programs from the `Executables` directory
   - Example: `http://localhost:8080/?file=echo&arguments=Hello%20World`

3. **Directory Browsing**: `http://localhost:8080/?dir=directory_name`
   - Browse directories within `serving_files`

4. **File Download**: `http://localhost:8080/?dir=directory&file=filename`
   - Download specific files through the browser interface

## Technical Details

### Thread Pool Implementation
- Creates 4 worker threads on startup
- Uses POSIX threads (pthread) for concurrency
- Implements a task queue with condition variables for efficient thread synchronization
- Each connection is handled by an available worker thread

### Request Processing
1. Accepts incoming TCP connections on port 8080
2. Enqueues connections to the thread pool
3. Worker threads process HTTP requests
4. Parses GET requests and determines action (serve file, execute command, run PHP)
5. Sends appropriate HTTP response with content

### Command Execution
- Uses `posix_spawn()` for secure process creation
- Captures stdout from executed programs
- Supports both custom executables and system binaries (from `/bin/`)
- Output is processed through PHP for HTML formatting

### Supported Content Types
- HTML (`text/html`)
- CSS (`text/css`)
- JavaScript (`application/javascript`)
- PNG images (`image/png`)
- JPEG images (`image/jpeg`, `image/jpg`)
- GIF images (`image/gif`)
- Plain text (default)

## Security Considerations

- Path sanitization removes `../` patterns to prevent directory traversal
- Executables are restricted to the `Executables` directory and `/bin/`
- PHP scripts have built-in directory traversal protection
- File serving is restricted to the `serving_files` directory

## Dependencies

- C++11 or later
- POSIX threads library
- PHP CLI (for PHP script execution)
- Standard UNIX utilities (for system commands)

## Limitations

- Fixed port (8080) - not configurable without recompilation
- Fixed thread pool size (4 threads)
- No HTTPS support
- Basic HTTP/1.1 implementation
- No support for POST requests or file uploads

## Error Handling

- Returns 404 for non-existent files
- Handles socket errors gracefully
- Logs errors to stderr
- Closes connections properly on errors

## Future Improvements

- Configuration file support for port and thread pool size
- HTTPS/TLS support
- POST request handling
- File upload capability
- Request logging to file
- Rate limiting and DOS protection
- WebSocket support
- HTTP/2 support