#include <iostream>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ; // Environment variables

int main() {
    pid_t pid; // To store the PID of the child process
    int status; // To store the return status of the child process

    // The child processes path and argument
    char *program = (char *) "./Executables/print_arg";
    char *arg = (char *) "Hello World!";

    char *argv[] = {program, arg , NULL};

    // Spawn a new process
    status = posix_spawn(&pid, argv[0], NULL, NULL, argv, environ);

    if (status == 0) {
        // Child process created successfully
        std::cout << "Child process created with PID: " << pid << std::endl;

        // Wait for the child process to finish
        if (waitpid(pid, &status, 0) != -1) {
            std::cout << "Child process exited with status: " << status << std::endl;
        } else {
            perror("waitpid"); // Error during waiting
        }
    } else {
        std::cerr << "posix_spawn failed with error: " << status << std::endl; // Process creation failed
    }

    return 0; // Exit the parent process
}
