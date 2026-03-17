/*
 * CloudTaser Interactive Demo
 * Fullscreen TUI that guides users through the demo steps.
 * Cross-compile: x86_64-linux-musl-gcc -static -Os -s -o demo/assets/demo demo.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

/* ── ANSI helpers ────────────────────────────────────────────────── */
#define CSI        "\033["
#define CLEAR      CSI "2J" CSI "H"
#define HIDE_CUR   CSI "?25l"
#define SHOW_CUR   CSI "?25h"
#define BOLD       CSI "1m"
#define DIM        CSI "2m"
#define RESET      CSI "0m"
#define FG_WHITE   CSI "97m"
#define FG_GREEN   CSI "32m"
#define FG_RED     CSI "31m"
#define FG_CYAN    CSI "36m"
#define FG_YELLOW  CSI "33m"
#define FG_GRAY    CSI "90m"
#define BG_BLUE    CSI "44m"
#define BG_GREEN   CSI "42m"
#define BG_RED     CSI "41m"
#define BG_GRAY    CSI "100m"

/* 256-color for lightning effect */
static const int lightning[] = {75, 75, 111, 153, 231, 231, 153, 111, 75, 75};
#define N_LIGHTNING 10

/* ── Terminal ────────────────────────────────────────────────────── */
static struct termios orig_termios;
static int term_w = 80, term_h = 24;

static void get_term_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_w = ws.ws_col;
        term_h = ws.ws_row;
    }
}

static void raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;  /* 100ms timeout */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void restore_term(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf(SHOW_CUR RESET);
    fflush(stdout);
}

static int read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return (unsigned char)c;
    return -1;
}

/* blocking read */
static int wait_key(void) {
    int k;
    while ((k = read_key()) < 0) usleep(50000);
    return k;
}

/* ── Drawing helpers ─────────────────────────────────────────────── */
static void move_to(int row, int col) {
    printf(CSI "%d;%dH", row, col);
}

static void hline(int row, int col, int len, char ch) {
    move_to(row, col);
    for (int i = 0; i < len; i++) putchar(ch);
}

static void draw_box(int top, int left, int w, int h) {
    /* top border */
    move_to(top, left);
    putchar('+');
    for (int i = 0; i < w - 2; i++) putchar('-');
    putchar('+');
    /* sides */
    for (int r = 1; r < h - 1; r++) {
        move_to(top + r, left);
        putchar('|');
        for (int i = 0; i < w - 2; i++) putchar(' ');
        putchar('|');
    }
    /* bottom border */
    move_to(top + h - 1, left);
    putchar('+');
    for (int i = 0; i < w - 2; i++) putchar('-');
    putchar('+');
}

/* Print text centered on a row, within a box of width `bw` starting at `bleft` */
static void print_centered(int row, int bleft, int bw, const char *text) {
    int len = (int)strlen(text);
    int pad = (bw - 2 - len) / 2;
    if (pad < 0) pad = 0;
    move_to(row, bleft + 1 + pad);
    printf("%s", text);
}

/* Print text left-aligned inside box */
static void print_left(int row, int bleft, int bw, const char *text) {
    move_to(row, bleft + 2);
    /* truncate to box width */
    int maxlen = bw - 4;
    if (maxlen < 0) maxlen = 0;
    printf("%.*s", maxlen, text);
}

/* Print wrapped text, returns number of rows used */
static int print_wrapped(int start_row, int bleft, int bw, const char *text) {
    int maxlen = bw - 4;
    if (maxlen < 1) maxlen = 1;
    int len = (int)strlen(text);
    int rows = 0;
    int pos = 0;
    while (pos < len) {
        int chunk = len - pos;
        if (chunk > maxlen) chunk = maxlen;
        /* try to break at word boundary */
        if (pos + chunk < len) {
            int brk = chunk;
            while (brk > 0 && text[pos + brk] != ' ') brk--;
            if (brk > 0) chunk = brk;
        }
        move_to(start_row + rows, bleft + 2);
        printf("%.*s", chunk, text + pos);
        pos += chunk;
        if (pos < len && text[pos] == ' ') pos++;
        rows++;
    }
    return rows > 0 ? rows : 1;
}

/* ── Lightning "CloudTaser" title ────────────────────────────────── */
static void draw_title(int row, int bleft, int bw, int frame) {
    const char *prefix = "Cloud";
    const char *taser = "Taser";
    int total = 10 + 5 + 15; /* "CloudTaser Interactive Demo" */
    int pad = (bw - 2 - 27) / 2;
    if (pad < 0) pad = 0;
    move_to(row, bleft + 1 + pad);
    printf(BOLD FG_WHITE "%s", prefix);
    for (int i = 0; i < 5; i++) {
        int ci = (frame + i * 2) % N_LIGHTNING;
        printf(CSI "38;5;%dm%c", lightning[ci], taser[i]);
    }
    printf(RESET BOLD FG_WHITE " Interactive Demo" RESET);
}

/* ── Step definitions ───────────────────────────────────────────── */
typedef struct {
    const char *title;
    const char *description[8]; /* up to 8 lines of description, NULL terminated */
    const char *commands[6];    /* commands to run, NULL terminated */
    const char *check_cmd;      /* command to validate success (exit 0 = pass) */
} Step;

static Step steps[] = {
    {
        "Deploy a Protected PostgreSQL Pod",
        {
            "CloudTaser's webhook intercepts pod creation and injects",
            "a thin wrapper. The operator acts as an auth broker —",
            "it holds one EU Vault token and creates scoped child",
            "tokens for each pod automatically.",
            "",
            "We'll deploy a postgres pod with CloudTaser annotations.",
            "No secrets in the manifest — they come from EU Vault.",
            NULL
        },
        {
            "cat /tmp/postgres-demo.yaml",
            "kubectl apply -f /tmp/postgres-demo.yaml",
            "kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s",
            "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].command}' | python3 -m json.tool",
            NULL
        },
        "kubectl get pod postgres-demo -o jsonpath='{.status.phase}' | grep -q Running"
    },
    {
        "Verify Secrets Are Not in Kubernetes",
        {
            "The whole point of CloudTaser is that secrets never",
            "touch Kubernetes storage. No K8s Secrets, no etcd.",
            "",
            "We'll check both the Kubernetes API and etcd directly",
            "to prove POSTGRES_PASSWORD is nowhere to be found.",
            NULL
        },
        {
            "kubectl get secrets -n default",
            "kubectl exec -n kube-system etcd-controlplane -- etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key get \"\" --prefix --keys-only | grep -i postgres_password || echo \"Not found in etcd - secrets are safe\"",
            "kubectl get pod postgres-demo -o jsonpath='{.spec.containers[0].env[*].name}'",
            NULL
        },
        "kubectl exec -n kube-system etcd-controlplane -- etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key get '' --prefix --keys-only 2>/dev/null | grep -qi postgres_password; test $? -ne 0"
    },
    {
        "Confirm Secrets Work Inside the Pod",
        {
            "Even though secrets aren't in Kubernetes, the app has",
            "full access. PostgreSQL requires POSTGRES_PASSWORD to",
            "start — and it's running. The wrapper fetched the",
            "password from EU Vault and injected it into memory.",
            NULL
        },
        {
            "kubectl exec postgres-demo -- psql -U postgres -c \"SELECT 'Connected successfully' as status;\"",
            "kubectl logs postgres-demo -c postgres | grep -E \"secrets loaded|fetching|unsealed\"",
            NULL
        },
        "kubectl exec postgres-demo -- psql -U postgres -c 'SELECT 1' > /dev/null 2>&1"
    },
    {
        "Try to Read /proc/pid/environ",
        {
            "This is the key security moment. On a normal K8s node,",
            "anyone with host access can read environment variables",
            "via /proc/<pid>/environ. CloudTaser's eBPF agent blocks",
            "this at the kernel level with kprobe enforcement.",
            "",
            "The openat syscall returns -EACCES BEFORE any data",
            "is read. Zero data leakage.",
            NULL
        },
        {
            "PROTECTED_PID=$(kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | python3 -c \"\nimport sys, json\nfor line in sys.stdin:\n    try:\n        evt = json.loads(line.strip())\n        if 'registered PID' in evt.get('msg', ''):\n            pid = evt.get('host_pid', '')\n            if pid: print(pid)\n    except: pass\n\" | tail -1) && echo \"Protected host PID: $PROTECTED_PID\"",
            "cat /proc/${PROTECTED_PID}/environ 2>&1; echo \"Exit code: $?\"",
            NULL
        },
        "cat /proc/$(kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | python3 -c \"import sys, json\nfor line in sys.stdin:\n    try:\n        evt = json.loads(line.strip())\n        if 'registered PID' in evt.get('msg', ''):\n            pid = evt.get('host_pid', '')\n            if pid: print(pid)\n    except: pass\" | tail -1)/environ 2>/dev/null; test $? -ne 0"
    },
    {
        "View the Audit Trail",
        {
            "CloudTaser logs every security event. The blocked",
            "/proc/environ read was recorded with full context:",
            "PID, command, timestamp, and severity.",
            "",
            "In production, events forward to your SIEM platform,",
            "providing the audit trail required by GDPR, NIS2, DORA.",
            NULL
        },
        {
            "kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | grep -E \"ENVIRON|blocked\"",
            NULL
        },
        "kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 2>/dev/null | grep -qi environ"
    },
};
#define N_STEPS (int)(sizeof(steps) / sizeof(steps[0]))

/* ── Run a command and capture output ────────────────────────────── */
#define MAX_OUTPUT 8192

static int run_command(const char *cmd, char *output, int max_out) {
    int pipefd[2];
    if (pipe(pipefd) < 0) return -1;

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("/bin/bash", "bash", "-c", cmd, NULL);
        _exit(127);
    }
    close(pipefd[1]);

    int total = 0;
    char buf[512];
    int n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        int to_copy = n;
        if (total + to_copy >= max_out - 1)
            to_copy = max_out - 1 - total;
        if (to_copy > 0) {
            memcpy(output + total, buf, to_copy);
            total += to_copy;
        }
    }
    output[total] = '\0';
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* ── Draw a button ───────────────────────────────────────────────── */
static void draw_button(int row, int col, const char *label, const char *color, int selected) {
    move_to(row, col);
    if (selected)
        printf(BOLD "%s" FG_WHITE " [ %s ] " RESET, color, label);
    else
        printf(DIM FG_GRAY " [ %s ] " RESET, label);
}

/* ── Flashing Next button ────────────────────────────────────────── */
static void draw_next_flash(int row, int col, int frame, int success) {
    move_to(row, col);
    if (success) {
        /* flash between bright green and dim green */
        if ((frame / 3) & 1)
            printf(BOLD BG_GREEN FG_WHITE " [ NEXT >> ] " RESET);
        else
            printf(BOLD FG_GREEN " [ NEXT >> ] " RESET);
    } else {
        printf(BOLD FG_WHITE " [ NEXT >> ] " RESET);
    }
}

/* ── Main page renderer ─────────────────────────────────────────── */
static void draw_page(int step_idx, int cmd_idx, int cmd_done,
                      const char *output, int result, int frame) {
    Step *s = &steps[step_idx];
    get_term_size();

    int bw = term_w;
    int bh = term_h;
    int bleft = 1;
    int btop = 1;

    if (bw > 100) { bw = 100; bleft = (term_w - bw) / 2 + 1; }

    printf(CLEAR HIDE_CUR);

    /* Box */
    draw_box(btop, bleft, bw, bh);

    /* Title row */
    draw_title(btop + 1, bleft, bw, frame);

    /* Step indicator */
    {
        char stepbar[64];
        snprintf(stepbar, sizeof(stepbar), "Step %d of %d", step_idx + 1, N_STEPS);
        move_to(btop + 2, bleft + bw - 2 - (int)strlen(stepbar));
        printf(DIM "%s" RESET, stepbar);
    }

    /* Divider */
    hline(btop + 3, bleft + 1, bw - 2, '-');

    /* Step title */
    move_to(btop + 4, bleft + 2);
    printf(BOLD FG_CYAN "%s" RESET, s->title);

    /* Description */
    int row = btop + 6;
    for (int i = 0; s->description[i]; i++) {
        if (s->description[i][0] == '\0') {
            row++;
            continue;
        }
        print_left(row, bleft, bw, s->description[i]);
        row++;
    }

    /* Divider */
    row++;
    hline(row, bleft + 1, bw - 2, '-');
    row++;

    /* Commands section */
    int total_cmds = 0;
    while (s->commands[total_cmds]) total_cmds++;

    if (cmd_idx < total_cmds && !cmd_done) {
        /* Show current command to run */
        move_to(row, bleft + 2);
        printf(FG_YELLOW "Command %d of %d:" RESET, cmd_idx + 1, total_cmds);
        row++;
        row++;

        /* Show command (potentially multi-line, show first line with $ prefix) */
        const char *cmd = s->commands[cmd_idx];
        move_to(row, bleft + 3);
        printf(BOLD FG_WHITE "$ " RESET);
        /* show first 70 chars or to first newline */
        int cmdlen = (int)strlen(cmd);
        int show = cmdlen;
        const char *nl = strchr(cmd, '\n');
        if (nl) show = (int)(nl - cmd);
        if (show > bw - 8) show = bw - 8;
        printf(FG_WHITE "%.*s" RESET, show, cmd);
        if (nl || cmdlen > show) {
            move_to(row + 1, bleft + 5);
            printf(DIM "(continued...)" RESET);
        }
        row += 3;

        /* Run button */
        int btn_col = (bw - 24) / 2 + bleft;
        draw_button(row, btn_col, "RUN", BG_GREEN, 1);
        draw_button(row, btn_col + 14, "SKIP", BG_GRAY, 0);
        row += 2;

        /* Hint */
        print_centered(row, bleft, bw, "Press ENTER to run, 's' to skip");
    } else if (cmd_done || cmd_idx >= total_cmds) {
        /* Show output area */
        move_to(row, bleft + 2);
        printf(FG_YELLOW "Output:" RESET);
        row++;

        /* Show output lines (limited to available space) */
        int out_start = row;
        int max_rows = bh - (row - btop) - 6;
        if (max_rows < 3) max_rows = 3;

        if (output && output[0]) {
            const char *p = output;
            int lines = 0;
            /* count total lines */
            int total_lines = 1;
            for (const char *q = output; *q; q++)
                if (*q == '\n') total_lines++;
            /* skip to show last N lines if too many */
            int skip = total_lines - max_rows;
            if (skip < 0) skip = 0;

            int cur_line = 0;
            while (*p && lines < max_rows) {
                const char *eol = strchr(p, '\n');
                int llen = eol ? (int)(eol - p) : (int)strlen(p);
                if (cur_line >= skip) {
                    move_to(out_start + lines, bleft + 3);
                    int show = llen;
                    if (show > bw - 6) show = bw - 6;
                    printf(FG_WHITE "%.*s" RESET, show, p);
                    lines++;
                }
                cur_line++;
                p = eol ? eol + 1 : p + llen;
                if (!eol) break;
            }
            row = out_start + lines;
        } else {
            move_to(row, bleft + 3);
            printf(DIM "(no output)" RESET);
            row++;
        }

        row++;

        /* Result assessment and Next */
        if (cmd_idx >= total_cmds) {
            /* All commands done — show result */
            int res_row = bh + btop - 4;
            if (result == 0) {
                move_to(res_row, bleft + 2);
                printf(BOLD FG_GREEN "Result: PASS" RESET);
            } else if (result > 0) {
                move_to(res_row, bleft + 2);
                printf(BOLD FG_RED "Result: UNEXPECTED (exit %d)" RESET, result);
            }

            /* Next / Finish button */
            int next_row = bh + btop - 2;
            int btn_col = bleft + bw - 18;
            if (step_idx < N_STEPS - 1)
                draw_next_flash(next_row, btn_col, frame, result == 0);
            else {
                move_to(next_row, btn_col);
                printf(BOLD BG_GREEN FG_WHITE " [ FINISH ] " RESET);
            }
            move_to(next_row - 1, bleft + 2);
            printf(DIM "Press ENTER to continue" RESET);
        } else {
            /* More commands to run */
            int btn_row = bh + btop - 3;
            int btn_col = (bw - 24) / 2 + bleft;
            draw_button(btn_row, btn_col, "RUN NEXT", BG_GREEN, 1);
            draw_button(btn_row, btn_col + 16, "SKIP", BG_GRAY, 0);
            move_to(btn_row + 1, bleft + 2);
            printf(DIM "Press ENTER to run, 's' to skip" RESET);
        }
    }

    /* Progress bar at bottom */
    {
        int prow = bh + btop - 1;
        move_to(prow, bleft + 1);
        int bar_w = bw - 2;
        int filled = (step_idx * bar_w) / N_STEPS;
        printf(BG_BLUE);
        for (int i = 0; i < filled; i++) putchar(' ');
        printf(RESET);
    }

    fflush(stdout);
}

/* ── Finish screen ──────────────────────────────────────────────── */
static void draw_finish(int frame) {
    get_term_size();
    int bw = term_w;
    int bh = term_h;
    int bleft = 1;
    int btop = 1;
    if (bw > 100) { bw = 100; bleft = (term_w - bw) / 2 + 1; }

    printf(CLEAR HIDE_CUR);
    draw_box(btop, bleft, bw, bh);
    draw_title(btop + 1, bleft, bw, frame);
    hline(btop + 3, bleft + 1, bw - 2, '-');

    int row = btop + 5;
    move_to(row, bleft + 2);
    printf(BOLD FG_GREEN "Demo Complete!" RESET);
    row += 2;

    const char *summary[] = {
        "You've seen CloudTaser in action:",
        "",
        "  * Secrets never touch Kubernetes — no etcd, no K8s Secrets",
        "  * Applications work normally — secrets available in memory",
        "  * eBPF enforcement blocks /proc/environ reads at kernel level",
        "  * Full audit trail — every event logged for compliance",
        "",
        "CloudTaser vs alternatives:",
        "",
        "  K8s Secrets          -> secrets in etcd (readable by provider)",
        "  External Secrets Op  -> secrets still land in etcd",
        "  Sealed Secrets       -> decrypted into etcd",
        "  CloudTaser           -> secrets ONLY in process memory",
        "",
        "",
        "Learn more:    cloudtaser.io",
        "Contact us:    cloud@skipops.ltd",
        NULL
    };

    for (int i = 0; summary[i]; i++) {
        print_left(row + i, bleft, bw, summary[i]);
    }

    int btn_row = bh + btop - 3;
    int btn_col = (bw - 12) / 2 + bleft;
    move_to(btn_row, btn_col);
    if ((frame / 4) & 1)
        printf(BOLD BG_GREEN FG_WHITE " [ EXIT ] " RESET);
    else
        printf(BOLD FG_GREEN " [ EXIT ] " RESET);
    move_to(btn_row + 1, bleft + 2);
    printf(DIM "Press ENTER or 'q' to exit" RESET);

    fflush(stdout);
}

/* ── Waiting screen with lightning animation ─────────────────────── */
static void wait_for_setup(void) {
    struct stat st;
    int frame = 0;
    const char spin[] = "|/-\\";

    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        get_term_size();
        int bw = term_w;
        int bleft = 1;
        if (bw > 100) { bw = 100; bleft = (term_w - bw) / 2 + 1; }

        int row = term_h / 2;
        move_to(row, 1);
        /* clear line */
        for (int i = 0; i < term_w; i++) putchar(' ');

        int pad = (bw - 30) / 2 + bleft;
        move_to(row, pad);
        printf(BOLD FG_WHITE "Installing Cloud");
        const char *taser = "Taser";
        for (int i = 0; i < 5; i++) {
            int ci = (frame + i * 2) % N_LIGHTNING;
            printf(CSI "38;5;%dm%c", lightning[ci], taser[i]);
        }
        printf(RESET BOLD FG_WHITE " %c" RESET, spin[frame & 3]);
        fflush(stdout);
        frame++;
        usleep(120000);
    }

    /* done */
    get_term_size();
    int row = term_h / 2;
    move_to(row, 1);
    for (int i = 0; i < term_w; i++) putchar(' ');
    int bw = term_w;
    int bleft = 1;
    if (bw > 100) { bw = 100; bleft = (term_w - bw) / 2 + 1; }
    int pad = (bw - 30) / 2 + bleft;
    move_to(row, pad);
    printf(BOLD FG_WHITE "Installing Cloud" CSI "38;5;231mTaser" RESET
           BOLD FG_GREEN " done" RESET "\n");
    fflush(stdout);
    usleep(800000);
}

/* ── Main ────────────────────────────────────────────────────────── */
int main(void) {
    printf(HIDE_CUR);
    fflush(stdout);

    /* Phase 1: Wait for setup */
    wait_for_setup();

    /* Phase 2: Interactive demo */
    raw_mode();
    atexit(restore_term);

    int step = 0;
    int cmd = 0;
    int cmd_done = 0;
    char output[MAX_OUTPUT] = "";
    int result = -1;
    int frame = 0;

    while (step < N_STEPS) {
        draw_page(step, cmd, cmd_done, output, result, frame);
        frame++;

        int key = read_key();
        if (key == 'q' || key == 3) {  /* q or Ctrl-C */
            break;
        }

        Step *s = &steps[step];
        int total_cmds = 0;
        while (s->commands[total_cmds]) total_cmds++;

        if (cmd < total_cmds && !cmd_done) {
            /* Waiting for user to run current command */
            if (key == '\n' || key == '\r') {
                /* Run the command */
                restore_term();
                printf(CLEAR);
                move_to(1, 1);
                printf(BOLD FG_CYAN "Running: " RESET FG_WHITE);
                /* show short version */
                const char *c = s->commands[cmd];
                const char *nl = strchr(c, '\n');
                int show = nl ? (int)(nl - c) : (int)strlen(c);
                if (show > 70) show = 70;
                printf("%.*s", show, c);
                if (nl || (int)strlen(c) > 70) printf("...");
                printf(RESET "\n\n");
                fflush(stdout);

                /* run with visible output */
                int pipefd[2];
                pipe(pipefd);
                pid_t pid = fork();
                if (pid == 0) {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDERR_FILENO);
                    close(pipefd[1]);
                    execl("/bin/bash", "bash", "-c", s->commands[cmd], NULL);
                    _exit(127);
                }
                close(pipefd[1]);

                /* read and display output simultaneously */
                int out_len = 0;
                char buf[512];
                int n;
                while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
                    write(STDOUT_FILENO, buf, n);
                    int to_copy = n;
                    if (out_len + to_copy >= MAX_OUTPUT - 1)
                        to_copy = MAX_OUTPUT - 1 - out_len;
                    if (to_copy > 0) {
                        memcpy(output + out_len, buf, to_copy);
                        out_len += to_copy;
                    }
                }
                output[out_len] = '\0';
                close(pipefd[0]);

                int status;
                waitpid(pid, &status, 0);
                result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

                raw_mode();
                cmd++;
                cmd_done = 0;

                /* small pause to see output */
                usleep(500000);

            } else if (key == 's' || key == 'S') {
                /* Skip this command */
                cmd++;
                output[0] = '\0';
            }
        } else if (cmd >= total_cmds) {
            /* All commands done, run check and wait for NEXT */
            if (result == -1 && s->check_cmd) {
                char chk_out[256];
                result = run_command(s->check_cmd, chk_out, sizeof(chk_out));
            } else if (result == -1) {
                result = 0; /* no check = pass */
            }

            if (key == '\n' || key == '\r') {
                /* Next step */
                step++;
                cmd = 0;
                cmd_done = 0;
                output[0] = '\0';
                result = -1;
            }
        }

        usleep(100000); /* 100ms frame time */
    }

    /* Finish screen */
    if (step >= N_STEPS) {
        frame = 0;
        while (1) {
            draw_finish(frame);
            frame++;
            int key = read_key();
            if (key == '\n' || key == '\r' || key == 'q' || key == 3)
                break;
            usleep(100000);
        }
    }

    printf(CLEAR SHOW_CUR);
    return 0;
}
