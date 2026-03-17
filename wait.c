#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    struct stat st;
    const char spin[] = "|/-\\";
    int i = 0;
    printf("Preparing the demo  ");
    fflush(stdout);
    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        printf("\b%c", spin[i++ & 3]);
        fflush(stdout);
        usleep(200000);
    }
    printf("\bdone\n");
    return 0;
}
