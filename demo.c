/*
 * CloudTaser Interactive Demo — Fullscreen TUI
 * Split layout: left=product summary, right=interactive steps
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

/* ── ANSI ────────────────────────────────────────────────────────── */
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
#define BG_GREEN   CSI "42m"
#define BG_GRAY    CSI "100m"
#define BG_BLUE    CSI "44m"

/* ── Terminal ────────────────────────────────────────────────────── */
static struct termios orig_termios;
static int tw = 80, th = 24;

static void get_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        tw = ws.ws_col; th = ws.ws_row;
    }
}

static void raw_on(void) {
    struct termios r;
    tcgetattr(STDIN_FILENO, &orig_termios);
    r = orig_termios;
    r.c_lflag &= ~(ICANON | ECHO);
    r.c_cc[VMIN] = 0;
    r.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &r);
}

static void raw_off(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf(SHOW_CUR RESET);
    fflush(stdout);
}

/* returns: ascii char, or 1000+key for special (1000=left,1001=right,1002=up,1003=down) */
#define K_LEFT  1000
#define K_RIGHT 1001
#define K_UP    1002
#define K_DOWN  1003

static int readkey(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c == 27) {
        unsigned char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return 27;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return 27;
        if (seq[0] == '[') {
            switch (seq[1]) {
            case 'D': return K_LEFT;
            case 'C': return K_RIGHT;
            case 'A': return K_UP;
            case 'B': return K_DOWN;
            }
        }
        return 27;
    }
    return c;
}

/* ── Drawing primitives ──────────────────────────────────────────── */
static void mv(int r, int c) { printf(CSI "%d;%dH", r, c); }

static void fill(int r, int c, int n, char ch) {
    mv(r, c);
    for (int i = 0; i < n; i++) putchar(ch);
}

static void vline(int r, int c, int n) {
    for (int i = 0; i < n; i++) { mv(r + i, c); putchar('|'); }
}

/* ── Run command, capture output ─────────────────────────────────── */
#define MAX_OUT 16384

static int run_cmd(const char *cmd, char *out, int maxlen) {
    int pfd[2];
    if (pipe(pfd) < 0) return -1;
    pid_t p = fork();
    if (p == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        dup2(pfd[1], STDERR_FILENO);
        close(pfd[1]);
        execl("/bin/bash", "bash", "-c", cmd, NULL);
        _exit(127);
    }
    close(pfd[1]);
    int total = 0;
    char buf[512];
    int n;
    while ((n = read(pfd[0], buf, sizeof(buf))) > 0) {
        int cp = n;
        if (total + cp >= maxlen - 1) cp = maxlen - 1 - total;
        if (cp > 0) { memcpy(out + total, buf, cp); total += cp; }
    }
    out[total] = '\0';
    close(pfd[0]);
    int st;
    waitpid(p, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* ── Step data ───────────────────────────────────────────────────── */
typedef struct {
    const char *title;
    const char *desc[6];
    const char *commands[6];
    const char *check;
} Step;

static Step steps[] = {
    {
        "Deploy a Protected PostgreSQL Pod",
        {
            "Deploy a postgres pod with CloudTaser annotations.",
            "No secrets in the manifest - they come from EU Vault.",
            "The operator auto-creates a scoped child token.",
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
            "Secrets never touch Kubernetes storage.",
            "No K8s Secrets, no etcd. Let's prove it.",
            NULL
        },
        {
            "kubectl get secrets -n default",
            "kubectl exec -n kube-system etcd-controlplane -- etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key get \"\" --prefix --keys-only | grep -i postgres_password || echo \"Not found in etcd - secrets are safe\"",
            NULL
        },
        "kubectl exec -n kube-system etcd-controlplane -- etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key get '' --prefix --keys-only 2>/dev/null | grep -qi postgres_password; test $? -ne 0"
    },
    {
        "Confirm Secrets Work Inside the Pod",
        {
            "PostgreSQL requires POSTGRES_PASSWORD to start.",
            "The wrapper fetched it from EU Vault into memory.",
            "App works normally - secrets are just invisible.",
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
            "On a normal K8s node, anyone with host access can",
            "read environment variables via /proc/<pid>/environ.",
            "CloudTaser's eBPF kprobe blocks this at kernel level.",
            "The openat syscall returns -EACCES. Zero data leakage.",
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
            "Every access attempt is logged with full context:",
            "PID, command, timestamp, severity.",
            "Events forward to SIEM for GDPR/NIS2/DORA compliance.",
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

/* ── Layout constants ────────────────────────────────────────────── */
#define LEFT_W 32  /* left panel width including border */

/* ── Static title with color ─────────────────────────────────────── */
static void draw_cloud_taser(int r, int c) {
    mv(r, c);
    printf(BOLD CSI "38;5;75mCloud" CSI "38;5;231mTaser" RESET);
}

/* ── Draw the full frame ─────────────────────────────────────────── */
static void draw_frame(int step, int cmd, int ncmds, int btn_sel,
                       const char *output, int check_result, int running) {
    get_size();
    printf(CLEAR HIDE_CUR);

    int W = tw, H = th;
    int lw = LEFT_W;
    if (lw > W / 3) lw = W / 3;
    int rw = W - lw;

    /* ── Top border ──────────────────────────────────────────────── */
    mv(1, 1);
    putchar('+');
    for (int i = 0; i < lw - 2; i++) putchar('-');
    putchar('+');
    for (int i = 0; i < rw - 1; i++) putchar('-');
    putchar('+');

    /* ── Side borders + divider ──────────────────────────────────── */
    for (int r = 2; r < H; r++) {
        mv(r, 1); putchar('|');
        mv(r, lw); putchar('|');
        mv(r, W); putchar('|');
    }

    /* ── Bottom border ───────────────────────────────────────────── */
    mv(H, 1);
    putchar('+');
    for (int i = 0; i < lw - 2; i++) putchar('-');
    putchar('+');
    for (int i = 0; i < rw - 1; i++) putchar('-');
    putchar('+');

    /* ═══════════════════════════════════════════════════════════════ */
    /* LEFT PANEL — product summary                                   */
    /* ═══════════════════════════════════════════════════════════════ */
    int lr = 2; /* current row in left panel */
    int lmax = lw - 4; /* max text width */

    draw_cloud_taser(lr, 3);
    lr += 2;

    mv(lr, 3); printf(DIM "EU Data Sovereignty" RESET);
    lr++;
    mv(lr, 3); printf(DIM "on US Cloud" RESET);
    lr += 2;

    const char *features[] = {
        BOLD FG_CYAN "*" RESET " Secrets " BOLD "never" RESET " in",
        "  Kubernetes etcd",
        "",
        BOLD FG_CYAN "*" RESET " Fetched from " BOLD "EU Vault" RESET,
        "  directly into memory",
        "",
        BOLD FG_CYAN "*" RESET " eBPF " BOLD "blocks" RESET,
        "  /proc/environ reads",
        "  at kernel level",
        "",
        BOLD FG_CYAN "*" RESET " Full " BOLD "audit trail" RESET,
        "  for GDPR, NIS2, DORA",
        "",
        BOLD FG_CYAN "*" RESET " CLOUD Act / FISA 702",
        "  " BOLD "resistant" RESET,
        NULL
    };
    for (int i = 0; features[i] && lr < H - 4; i++) {
        if (features[i][0] == '\0') { lr++; continue; }
        mv(lr, 3);
        printf("%s", features[i]);
        lr++;
    }

    /* bottom of left panel */
    mv(H - 3, 3); printf(FG_CYAN "cloudtaser.io" RESET);
    mv(H - 2, 3); printf(DIM "cloud@skipops.ltd" RESET);

    /* ═══════════════════════════════════════════════════════════════ */
    /* RIGHT PANEL — interactive step                                 */
    /* ═══════════════════════════════════════════════════════════════ */
    int rx = lw + 2;  /* left text position in right panel */
    int rmx = W - rx - 1; /* max text width */
    int rr = 2; /* current row */

    /* Step title */
    {
        char hdr[128];
        Step *s = &steps[step];
        snprintf(hdr, sizeof(hdr), "Step %d/%d: %s", step + 1, N_STEPS, s->title);
        mv(rr, rx);
        printf(BOLD FG_WHITE "%.*s" RESET, rmx, hdr);
        rr += 2;
    }

    /* Description */
    {
        Step *s = &steps[step];
        for (int i = 0; s->desc[i] && rr < 8; i++) {
            mv(rr, rx);
            printf("%.*s", rmx, s->desc[i]);
            rr++;
        }
        rr++;
    }

    /* Divider */
    fill(rr, lw + 1, rw - 1, '-');
    rr++;

    /* Command area */
    {
        Step *s = &steps[step];
        mv(rr, rx);
        if (cmd < ncmds) {
            printf(FG_YELLOW "Command %d/%d:" RESET, cmd + 1, ncmds);
        } else {
            printf(FG_GREEN "All commands completed" RESET);
        }
        rr++;

        if (cmd < ncmds) {
            mv(rr, rx);
            printf(BOLD "$ " RESET);
            const char *c = s->commands[cmd];
            const char *nl = strchr(c, '\n');
            int show = nl ? (int)(nl - c) : (int)strlen(c);
            if (show > rmx - 3) show = rmx - 3;
            printf(FG_WHITE "%.*s" RESET, show, c);
            if (nl || (int)strlen(c) > show) {
                rr++;
                mv(rr, rx + 2);
                printf(DIM "(...)" RESET);
            }
        }
        rr += 2;
    }

    /* Divider */
    fill(rr, lw + 1, rw - 1, '-');
    rr++;

    /* Output area */
    mv(rr, rx);
    printf(FG_YELLOW "Output:" RESET);
    rr++;

    if (running) {
        mv(rr, rx);
        printf(DIM "Running..." RESET);
    } else if (output && output[0]) {
        /* show output lines */
        int out_max_rows = H - rr - 5;
        if (out_max_rows < 2) out_max_rows = 2;
        const char *p = output;
        /* count lines */
        int total_lines = 0;
        for (const char *q = p; *q; q++) if (*q == '\n') total_lines++;
        if (output[strlen(output) - 1] != '\n') total_lines++;
        int skip = total_lines - out_max_rows;
        if (skip < 0) skip = 0;

        int cur = 0, shown = 0;
        while (*p && shown < out_max_rows) {
            const char *eol = strchr(p, '\n');
            int llen = eol ? (int)(eol - p) : (int)strlen(p);
            if (cur >= skip) {
                mv(rr + shown, rx);
                int show = llen;
                if (show > rmx) show = rmx;
                printf(FG_WHITE "%.*s" RESET, show, p);
                shown++;
            }
            cur++;
            p = eol ? eol + 1 : p + llen;
            if (!eol) break;
        }
    } else {
        mv(rr, rx);
        printf(DIM "(waiting for command)" RESET);
    }

    /* ── Result indicator ────────────────────────────────────────── */
    if (cmd >= ncmds && check_result >= 0) {
        mv(H - 4, rx);
        if (check_result == 0)
            printf(BOLD FG_GREEN "Result: PASS" RESET);
        else
            printf(BOLD FG_RED "Result: CHECK FAILED (exit %d)" RESET, check_result);
    }

    /* ── Buttons: [RUN] [NEXT] ───────────────────────────────────── */
    {
        int btn_row = H - 2;
        int center = lw + rw / 2;

        if (cmd < ncmds) {
            /* RUN is active, NEXT is gray */
            int run_col = center - 14;
            int next_col = center + 4;
            mv(btn_row, run_col);
            if (btn_sel == 0)
                printf(BOLD BG_GREEN FG_WHITE " [ RUN ] " RESET);
            else
                printf(BOLD FG_GREEN " [ RUN ] " RESET);
            mv(btn_row, next_col);
            printf(DIM FG_GRAY " [ NEXT >> ] " RESET);
        } else {
            /* All done — RUN gray, NEXT active */
            int run_col = center - 14;
            int next_col = center + 4;
            mv(btn_row, run_col);
            printf(DIM FG_GRAY " [ RUN ] " RESET);
            mv(btn_row, next_col);
            if (btn_sel == 1) {
                if (check_result == 0)
                    printf(BOLD BG_GREEN FG_WHITE " [ NEXT >> ] " RESET);
                else
                    printf(BOLD FG_WHITE " [ NEXT >> ] " RESET);
            } else {
                printf(BOLD FG_GREEN " [ NEXT >> ] " RESET);
            }
        }

        /* Hint */
        mv(btn_row + 1, center - 12);
        printf(DIM "Arrow keys to select, ENTER to confirm" RESET);
    }

    /* ── Progress bar ────────────────────────────────────────────── */
    {
        mv(H - 1, lw + 1);
        int bar_w = rw - 1;
        int filled = ((step + 1) * bar_w) / (N_STEPS + 1);
        for (int i = 0; i < bar_w; i++) {
            if (i < filled) printf(BG_BLUE " ");
            else printf(RESET DIM "." RESET);
        }
    }

    fflush(stdout);
}

/* ── Finish screen ──────────────────────────────────────────────── */
static void draw_finish(void) {
    get_size();
    printf(CLEAR HIDE_CUR);

    int W = tw, H = th;

    /* border */
    mv(1, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');
    for (int r = 2; r < H; r++) { mv(r, 1); putchar('|'); mv(r, W); putchar('|'); }
    mv(H, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');

    int cx = W / 2;
    int r = 3;

    draw_cloud_taser(r, cx - 5); r += 2;

    mv(r, cx - 8); printf(BOLD FG_GREEN "Demo Complete!" RESET); r += 2;

    const char *lines[] = {
        "You've seen CloudTaser in action:",
        "",
        "  * Secrets never touch Kubernetes - no etcd, no K8s Secrets",
        "  * Applications work normally - secrets available in memory",
        "  * eBPF enforcement blocks /proc/environ at kernel level",
        "  * Full audit trail - every event logged for compliance",
        "",
        "                 CloudTaser vs Alternatives",
        "  +-----------------------+--------+----------+----------+",
        "  |                       | K8s    | External | Cloud    |",
        "  |                       | Secret | Secrets  | Taser    |",
        "  +-----------------------+--------+----------+----------+",
        "  | Secrets in etcd       |  YES   |   YES    |   NO     |",
        "  | /proc/environ blocked |  NO    |   NO     |   YES    |",
        "  | Provider can read     |  YES   |   YES    |   NO     |",
        "  | CLOUD Act resistant   |  NO    |   NO     |   YES    |",
        "  +-----------------------+--------+----------+----------+",
        "",
        NULL
    };
    for (int i = 0; lines[i]; i++) {
        mv(r, 4);
        printf("  %s", lines[i]);
        r++;
    }

    r++;
    mv(r, cx - 10); printf(FG_CYAN "cloudtaser.io" RESET);
    mv(r + 1, cx - 10); printf(DIM "cloud@skipops.ltd" RESET);

    int btn_row = H - 2;
    mv(btn_row, cx - 6);
    printf(BOLD BG_GREEN FG_WHITE " [ EXIT ] " RESET);
    mv(btn_row + 1, cx - 10);
    printf(DIM "Press ENTER or 'q' to exit" RESET);
    fflush(stdout);
}

/* ── Wait for setup ──────────────────────────────────────────────── */
static void wait_for_setup(void) {
    struct stat st;
    int frame = 0;
    const char spin[] = "|/-\\";

    printf(HIDE_CUR);
    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        get_size();
        int r = th / 2;
        mv(r, 1);
        for (int i = 0; i < tw; i++) putchar(' ');
        int pad = (tw - 28) / 2;
        if (pad < 1) pad = 1;
        mv(r, pad);
        printf(BOLD CSI "38;5;75mCloud" CSI "38;5;231mTaser" RESET);
        printf(BOLD FG_WHITE " Installing %c" RESET, spin[frame & 3]);
        fflush(stdout);
        frame++;
        usleep(120000);
    }
    get_size();
    int r = th / 2;
    mv(r, 1);
    for (int i = 0; i < tw; i++) putchar(' ');
    int pad = (tw - 28) / 2;
    if (pad < 1) pad = 1;
    mv(r, pad);
    printf(BOLD CSI "38;5;75mCloud" CSI "38;5;231mTaser" RESET);
    printf(BOLD FG_GREEN " Ready" RESET);
    fflush(stdout);
    usleep(800000);
}

/* ── Main ────────────────────────────────────────────────────────── */
int main(void) {
    /* Phase 1: wait for setup */
    wait_for_setup();

    /* Phase 2: interactive */
    raw_on();
    atexit(raw_off);

    int step = 0;
    char output[MAX_OUT] = "";
    int cmd = 0;
    int btn = 0; /* 0=RUN, 1=NEXT */
    int check_result = -1;
    int running = 0;

    while (step < N_STEPS) {
        Step *s = &steps[step];
        int ncmds = 0;
        while (s->commands[ncmds]) ncmds++;

        draw_frame(step, cmd, ncmds, btn, output, check_result, running);

        int key = readkey();
        if (key == 'q' || key == 3) break; /* q or Ctrl-C */

        if (key == K_LEFT || key == K_RIGHT) {
            btn = (btn == 0) ? 1 : 0;
            continue;
        }

        if (key == '\n' || key == '\r') {
            if (btn == 0 && cmd < ncmds) {
                /* RUN current command */
                running = 1;
                draw_frame(step, cmd, ncmds, btn, output, check_result, running);
                running = 0;

                char cmd_out[MAX_OUT];
                run_cmd(s->commands[cmd], cmd_out, MAX_OUT);

                /* append to output with separator */
                int olen = strlen(output);
                if (olen > 0 && olen < MAX_OUT - 2) {
                    output[olen++] = '\n';
                    output[olen] = '\0';
                }
                /* add "$ cmd" header */
                {
                    const char *c = s->commands[cmd];
                    const char *nl = strchr(c, '\n');
                    int show = nl ? (int)(nl - c) : (int)strlen(c);
                    if (show > 60) show = 60;
                    int space = MAX_OUT - 1 - olen;
                    int wrote = snprintf(output + olen, space, "$ %.*s%s\n",
                                         show, c, (nl || (int)strlen(c) > 60) ? "..." : "");
                    if (wrote > 0 && wrote < space) olen += wrote;
                }
                /* add output */
                {
                    int clen = strlen(cmd_out);
                    int space = MAX_OUT - 1 - olen;
                    if (clen > space) clen = space;
                    memcpy(output + olen, cmd_out, clen);
                    output[olen + clen] = '\0';
                }

                cmd++;
                /* auto-select NEXT when all commands done */
                if (cmd >= ncmds) {
                    btn = 1;
                    /* run check */
                    if (s->check) {
                        char chk[256];
                        check_result = run_cmd(s->check, chk, sizeof(chk));
                    } else {
                        check_result = 0;
                    }
                }
            } else if (btn == 1 && cmd >= ncmds) {
                /* NEXT step */
                step++;
                cmd = 0;
                btn = 0;
                output[0] = '\0';
                check_result = -1;
            }
        }
    }

    /* Finish */
    if (step >= N_STEPS) {
        draw_finish();
        while (1) {
            int key = readkey();
            if (key == '\n' || key == '\r' || key == 'q' || key == 3)
                break;
            usleep(100000);
        }
    }

    printf(CLEAR SHOW_CUR);
    return 0;
}
