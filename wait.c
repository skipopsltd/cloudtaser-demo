#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    struct stat st;
    printf("Preparing the demo");
    fflush(stdout);
    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        printf(".");
        fflush(stdout);
        sleep(2);
    }
    printf("done\n");
    return 0;
}
