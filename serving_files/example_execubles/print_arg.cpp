#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // Check if there are more than one argument (excluding program name)
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }

    const char *str = argv[1];
    size_t size = strlen(str);

    struct timespec ts = {0, 250000000};

    // removed the nanosleep call because now the parent needs to wait for the child to finish entirely

    // Sleep for 0.5 seconds before doing anything
    //nanosleep(&ts, NULL);

    // Print the program name
    printf("\n%s started.\n\n", argv[0]);

    // Loop over each character in the input string
    // Print the character and sleep for 0.5 seconds
    for (size_t i = 0; i < size; i++) {
        printf("%c", str[i]);
        fflush(stdout);
      //  nanosleep(&ts, NULL);
    }

    printf("\nOutput orginated from PID: %d\n", getpid());

    // Display that the program has finished
    printf("\n%s finished.\n", argv[0]);

    return 0;
}
