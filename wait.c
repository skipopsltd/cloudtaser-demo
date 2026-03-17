#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    struct stat st;
    const char *taser = "Taser";
    /* blue-to-white lightning wave (256-color ANSI) */
    const int colors[] = {75, 75, 111, 153, 231, 231, 153, 111, 75, 75};
    int nc = 10;
    int frame = 0;
    const char spin[] = "|/-\\";

    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        printf("\rInstalling Cloud");
        for (int i = 0; i < 5; i++) {
            int ci = (frame + i * 2) % nc;
            printf("\033[38;5;%dm%c", colors[ci], taser[i]);
        }
        printf("\033[0m %c", spin[frame & 3]);
        fflush(stdout);
        frame++;
        usleep(120000);
    }
    printf("\rInstalling Cloud\033[38;5;231mTaser\033[0m done\n");
    return 0;
}
