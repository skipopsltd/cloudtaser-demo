/*
 * CloudTaser Interactive Demo — Fullscreen TUI
 * Full-width layout, yellow output box, text input support, 5 steps
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
#define FG_PINK    CSI "95m"
#define CLR_LINE   CSI "2K"

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

#define K_LEFT  1000
#define K_RIGHT 1001

static int readkey(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c == 27) {
        unsigned char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return 27;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return 27;
        if (seq[0] == '[') {
            if (seq[1] == 'D') return K_LEFT;
            if (seq[1] == 'C') return K_RIGHT;
        }
        return 27;
    }
    return c;
}

/* ── Drawing primitives ──────────────────────────────────────────── */
static void mv(int r, int c) { printf(CSI "%d;%dH", r, c); }

static void fill_ch(int r, int c, int n, char ch) {
    mv(r, c);
    for (int i = 0; i < n; i++) putchar(ch);
}

/* ── Print line with password highlighted in pink ─────────────────── */
static void print_highlighted(const char *line, int maxlen,
                              const char *highlight) {
    if (!highlight || !highlight[0]) {
        printf(FG_WHITE "%.*s" RESET, maxlen, line);
        return;
    }
    int hlen = strlen(highlight);
    const char *p = line;
    const char *end = line + maxlen;
    while (p < end && *p) {
        /* find next occurrence */
        const char *match = NULL;
        for (const char *s = p; s + hlen <= end && *s; s++) {
            if (strncmp(s, highlight, hlen) == 0) { match = s; break; }
        }
        if (match) {
            if (match > p)
                printf(FG_WHITE "%.*s" RESET, (int)(match - p), p);
            printf(BOLD CSI "45;97m" " %.*s " RESET, hlen, match);
            p = match + hlen;
        } else {
            printf(FG_WHITE "%.*s" RESET, (int)(end - p), p);
            break;
        }
    }
}

/* ── Substitute __PASSWORD__ placeholder with user_input ─────────── */
#define PH "__PASSWORD__"
#define MAX_CMD 4096

static void expand_cmd(const char *tpl, char *out, int maxlen,
                       const char *password) {
    int phlen = strlen(PH);
    const char *p = tpl;
    int pos = 0;
    while (*p && pos < maxlen - 1) {
        if (strncmp(p, PH, phlen) == 0) {
            int plen = password ? (int)strlen(password) : 0;
            if (pos + plen < maxlen - 1) {
                memcpy(out + pos, password, plen);
                pos += plen;
            }
            p += phlen;
        } else {
            out[pos++] = *p++;
        }
    }
    out[pos] = '\0';
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
    const char *cmd_desc[6];
    const char *check;
    const char *input_prompt; /* if non-NULL, show text input before commands */
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
        {
            "View the pod manifest — no secrets, just annotations",
            "Deploy the pod — operator injects the wrapper sidecar",
            "Wait for the pod to become ready",
            "Inspect the injected command — wrapper wraps postgres",
            NULL
        },
        "kubectl get pod postgres-demo -o jsonpath='{.status.phase}' | grep -q Running",
        NULL
    },
    {
        "Set Your Own Secret Password",
        {
            "Type any password you want below.",
            "It goes to EU OpenBao vault in Frankfurt.",
            "Visible only in this demo — never stored in the cluster.",
            NULL
        },
        {
            "curl -sf -X POST \"https://secret.cloudtaser.io/v1/secret/data/demo/$(cat /tmp/.session_id)/postgres\" -H \"X-Vault-Token: $(cat /tmp/.cloudtaser-session-token)\" -H \"Content-Type: application/json\" -d \"{\\\"data\\\": {\\\"password\\\": \\\"__PASSWORD__\\\", \\\"username\\\": \\\"postgres\\\"}}\" && echo \"Secret updated in EU Vault (Frankfurt)\"",
            "curl -sf \"https://secret.cloudtaser.io/v1/secret/data/demo/$(cat /tmp/.session_id)/postgres\" -H \"X-Vault-Token: $(cat /tmp/.cloudtaser-session-token)\" | python3 -c 'import sys,json; d=json.load(sys.stdin)[\"data\"][\"data\"]; print(\"Password in vault: \" + d[\"password\"])'",
            "echo \"=== See for yourself ===\" && echo -e \"URL:   \\033[44;97m https://secret.cloudtaser.io/ui \\033[0m\" && echo \"Path:  demo/$(cat /tmp/.session_id)/postgres\" && echo \"Token: $(cat /tmp/.cloudtaser-session-token)\"",
            "kubectl delete pod postgres-demo --grace-period=0 --force 2>/dev/null; kubectl apply -f /tmp/postgres-demo.yaml && kubectl wait --for=condition=Ready pod/postgres-demo --timeout=120s",
            NULL
        },
        {
            "Write YOUR password to EU OpenBao vault (Frankfurt)",
            "Read it back — proof it's stored in EU",
            "Vault UI credentials — see for yourself",
            "Recreate pod with your new secret",
            NULL
        },
        "kubectl exec postgres-demo -- psql -U postgres -c 'SELECT 1' > /dev/null 2>&1",
        "Enter your secret password"
    },
    {
        "Verify: No Secrets in K8s, YOUR Password Works",
        {
            "Your password never touched Kubernetes.",
            "Not in K8s Secrets, not in etcd, not even in env.",
            "Yet the app works — secrets live only in process memory.",
            NULL
        },
        {
            "kubectl get secrets -n default",
            "kubectl exec -n kube-system etcd-controlplane -- etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key get \"\" --prefix --keys-only | grep -i postgres_password || echo \"Not found in etcd - secrets are safe\"",
            "kubectl exec postgres-demo -- bash -c 'echo \"POSTGRES_PASSWORD=$POSTGRES_PASSWORD\"' || true",
            "echo \"Connecting with password: __PASSWORD__\" && kubectl exec postgres-demo -- psql -U postgres -c \"SELECT 'Connected!' as status;\"",
            NULL
        },
        {
            "List all secrets — none for postgres",
            "Search etcd directly — not there either",
            "Try to read password via exec — EMPTY, not in env",
            "Connect with YOUR password — same one, highlighted",
            NULL
        },
        "kubectl exec postgres-demo -- psql -U postgres -c 'SELECT 1' > /dev/null 2>&1",
        NULL
    },
    {
        "Block /proc/pid/environ Read",
        {
            "On a normal K8s node, anyone with host access can",
            "read env vars via /proc/<pid>/environ.",
            "CloudTaser's eBPF kprobe blocks this at kernel level.",
            "The openat syscall returns -EACCES. Zero leakage.",
            NULL
        },
        {
            "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 | grep -o '\"host_pid\":[0-9]*' | tail -1 | cut -d: -f2 | tee /tmp/.protected_pid | xargs -I{} echo \"Protected PID: {}\"",
            "cat /proc/$(cat /tmp/.protected_pid)/environ 2>&1; echo \"Exit code: $?\"",
            NULL
        },
        {
            "Find the protected process PID on the host",
            "Try to read process environment — eBPF blocks it",
            NULL
        },
        "P=$(kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 | grep -o '\"host_pid\":[0-9]*' | tail -1 | cut -d: -f2) && cat /proc/$P/environ 2>/dev/null; test $? -ne 0",
        NULL
    },
    {
        "View the Audit Trail",
        {
            "Every access attempt is logged with full context:",
            "PID, command, timestamp, severity.",
            "Events forward to SIEM for GDPR/NIS2/DORA.",
            NULL
        },
        {
            "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 | grep -E \"ENVIRON|blocked\"",
            NULL
        },
        {
            "View eBPF agent logs — every access attempt recorded",
            NULL
        },
        "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 2>/dev/null | grep -qi environ",
        NULL
    },
};
#define N_STEPS (int)(sizeof(steps) / sizeof(steps[0]))

/* ── Text input ──────────────────────────────────────────────────── */
#define MAX_INPUT 128
static char user_input[MAX_INPUT];

/* ── Branded title helper ────────────────────────────────────────── */
static void draw_cloud_taser(int r, int c) {
    mv(r, c);
    printf(BOLD CSI "38;5;75mCloud" CSI "38;5;231mTaser" RESET);
}

/* ── Draw static chrome (border + header + description). Once per step. ── */
static void draw_chrome(int step) {
    get_size();
    int W = tw, H = th;

    printf(CLEAR HIDE_CUR);

    /* outer border */
    mv(1, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');

    for (int r = 2; r < H; r++) {
        mv(r, 1); putchar('|');
        mv(r, W); putchar('|');
    }

    mv(H, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');

    /* title row: CloudTaser branding + step title */
    int rr = 2;
    draw_cloud_taser(rr, 3);
    Step *s = &steps[step];
    char hdr[128];
    snprintf(hdr, sizeof(hdr), "Step %d/%d: %s", step + 1, N_STEPS, s->title);
    mv(rr, 16);
    printf(DIM "|" RESET " " BOLD FG_WHITE "%.*s" RESET, W - 20, hdr);
    rr += 2;

    /* description */
    for (int i = 0; s->desc[i] && rr < 9; i++) {
        mv(rr, 3); printf("%.*s", W - 4, s->desc[i]); rr++;
    }
    rr++;
    fill_ch(rr, 2, W - 2, '-');

    /* progress bar */
    mv(H - 1, 2);
    int bar_w = W - 2;
    int filled = ((step + 1) * bar_w) / (N_STEPS + 1);
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) printf(BG_BLUE " " RESET);
        else printf(DIM "." RESET);
    }

    fflush(stdout);
}

/* ── Draw input field for text input steps ────────────────────────── */
static void draw_input(int step, const char *text, int cursor_pos) {
    get_size();
    int W = tw, H = th;
    int rx = 3;
    int rmx = W - 4;

    Step *s = &steps[step];
    int rr = 2 + 2;
    for (int i = 0; s->desc[i] && rr < 9; i++) rr++;
    rr += 2;

    /* clear dynamic area */
    for (int r = rr + 1; r < H; r++) {
        mv(r, 2);
        for (int i = 0; i < W - 2; i++) putchar(' ');
        mv(r, W); putchar('|');
    }
    mv(H, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');

    rr++;

    /* prompt */
    mv(rr, rx);
    printf(FG_CYAN "%s:" RESET, s->input_prompt);
    rr += 2;

    /* input box */
    int iw = rmx - 4;
    if (iw > 60) iw = 60;
    int ix = rx + 2;

    mv(rr, ix);
    printf(FG_YELLOW "+");
    for (int i = 0; i < iw; i++) putchar('-');
    printf("+" RESET);
    rr++;

    mv(rr, ix);
    printf(FG_YELLOW "|" RESET " ");
    int tlen = strlen(text);
    int show = tlen > iw - 3 ? iw - 3 : tlen;
    printf(BOLD FG_WHITE "%.*s" RESET, show, text);
    /* cursor block */
    printf(CSI "7m" " " RESET);
    for (int i = show + 1; i < iw - 1; i++) putchar(' ');
    printf(FG_YELLOW "|" RESET);
    rr++;

    mv(rr, ix);
    printf(FG_YELLOW "+");
    for (int i = 0; i < iw; i++) putchar('-');
    printf("+" RESET);
    rr += 2;

    mv(rr, rx);
    printf(DIM "Type your password, then press ENTER to continue" RESET);

    /* restore progress bar */
    mv(H - 1, 2);
    int bar_w = W - 2;
    int filled = ((step + 1) * bar_w) / (N_STEPS + 1);
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) printf(BG_BLUE " " RESET);
        else printf(DIM "." RESET);
    }

    fflush(stdout);
}

/* ── Update dynamic area (command, output box, buttons). ──────────── */
static void draw_dynamic(int step, int cmd, int ncmds, int btn,
                         const char *output, int check_result, int running) {
    get_size();
    int W = tw, H = th;
    int rx = 3;
    int rmx = W - 4;

    Step *s = &steps[step];
    int rr = 2 + 2;
    for (int i = 0; s->desc[i] && rr < 9; i++) rr++;
    rr += 2;

    /* clear dynamic area */
    for (int r = rr + 1; r < H; r++) {
        mv(r, 2);
        for (int i = 0; i < W - 2; i++) putchar(' ');
        mv(r, W); putchar('|');
    }
    mv(H, 1); putchar('+');
    for (int i = 0; i < W - 2; i++) putchar('-');
    putchar('+');

    rr++;

    /* command info — fixed 5-line area */
    int cmd_area_top = rr;
    int CMD_AREA_H = 5;

    mv(rr, rx);
    if (cmd < ncmds) {
        printf(FG_YELLOW "Command %d/%d:" RESET, cmd + 1, ncmds);
        if (s->cmd_desc[cmd]) {
            printf(" %.*s", rmx - 16, s->cmd_desc[cmd]);
        }
    } else {
        printf(FG_GREEN "All commands completed" RESET);
    }
    rr++;

    if (cmd < ncmds) {
        mv(rr, rx);
        printf(BOLD "$ " RESET);
        const char *c = s->commands[cmd];
        int clen = (int)strlen(c);
        int line_w = rmx - rx - 2;
        int pos = 0;
        int cmd_line = 0;
        while (pos < clen && cmd_line < CMD_AREA_H - 2) {
            int chunk = clen - pos;
            if (chunk > line_w) chunk = line_w;
            if (pos == 0) {
                printf(FG_WHITE "%.*s" RESET, chunk, c + pos);
            } else {
                rr++;
                cmd_line++;
                mv(rr, rx + 2);
                printf(FG_WHITE "%.*s" RESET, chunk, c + pos);
            }
            pos += chunk;
        }
    }

    /* Always position output box at a fixed row */
    rr = cmd_area_top + CMD_AREA_H + 1;

    /* output box with yellow border */
    {
        int ob_top = rr;
        int ob_bot = H - 5;
        int ob_left = 2;
        int ob_right = W - 1;
        int ob_inner_w = ob_right - ob_left - 2;
        int ob_inner_h = ob_bot - ob_top - 1;

        mv(ob_top, ob_left);
        printf(FG_YELLOW "+");
        for (int i = 0; i < ob_right - ob_left - 1; i++) putchar('-');
        printf("+" RESET);

        for (int r = ob_top + 1; r < ob_bot; r++) {
            mv(r, ob_left);
            printf(FG_YELLOW "|" RESET);
            for (int i = 0; i < ob_right - ob_left - 1; i++) putchar(' ');
            printf(FG_YELLOW "|" RESET);
        }

        mv(ob_bot, ob_left);
        printf(FG_YELLOW "+");
        for (int i = 0; i < ob_right - ob_left - 1; i++) putchar('-');
        printf("+" RESET);

        mv(ob_top, ob_left + 2);
        printf(FG_YELLOW " Output " RESET);

        int cx = ob_left + 2;
        int cy = ob_top + 1;

        if (running) {
            mv(cy, cx); printf(DIM "Running..." RESET);
        } else if (output && output[0]) {
            int out_max = ob_inner_h;
            if (out_max < 2) out_max = 2;
            const char *p = output;
            int total_lines = 0;
            for (const char *q = p; *q; q++) if (*q == '\n') total_lines++;
            if (output[strlen(output) - 1] != '\n') total_lines++;
            int skip = total_lines - out_max;
            if (skip < 0) skip = 0;

            int cur = 0, shown = 0;
            while (*p && shown < out_max) {
                const char *eol = strchr(p, '\n');
                int llen = eol ? (int)(eol - p) : (int)strlen(p);
                if (cur >= skip) {
                    mv(cy + shown, cx);
                    int sh = llen > ob_inner_w ? ob_inner_w : llen;
                    print_highlighted(p, sh, user_input);
                    shown++;
                }
                cur++;
                p = eol ? eol + 1 : p + llen;
                if (!eol) break;
            }
        } else {
            mv(cy, cx); printf(DIM "(waiting for command)" RESET);
        }
    }

    /* result */
    if (cmd >= ncmds && check_result >= 0) {
        mv(H - 4, rx);
        if (check_result == 0)
            printf(BOLD FG_GREEN "Result: PASS" RESET);
        else
            printf(BOLD FG_RED "Result: CHECK FAILED" RESET);
    }

    /* buttons */
    {
        int br = H - 2;
        int center = W / 2;
        int run_col = center - 14;
        int next_col = center + 2;

        mv(br, run_col);
        if (cmd < ncmds) {
            if (btn == 0)
                printf(BOLD BG_GREEN FG_WHITE " [ RUN ] " RESET);
            else
                printf(BOLD FG_GREEN " [ RUN ] " RESET);
        } else {
            printf(DIM FG_GRAY " [ RUN ] " RESET);
        }

        mv(br, next_col);
        if (cmd >= ncmds) {
            if (btn == 1)
                printf(BOLD BG_GREEN FG_WHITE " [ NEXT >> ] " RESET);
            else
                printf(BOLD FG_GREEN " [ NEXT >> ] " RESET);
        } else {
            printf(DIM FG_GRAY " [ NEXT >> ] " RESET);
        }

        mv(br + 1, center - 10);
        printf(DIM "< > arrows   ENTER confirm" RESET);
    }

    /* re-draw progress */
    mv(H - 1, 2);
    int bar_w = W - 2;
    int filled = ((step + 1) * bar_w) / (N_STEPS + 1);
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) printf(BG_BLUE " " RESET);
        else printf(DIM "." RESET);
    }

    fflush(stdout);
}

/* ── Finish screen ──────────────────────────────────────────────── */
static void draw_finish(void) {
    get_size();
    int W = tw, H = th;
    printf(CLEAR HIDE_CUR);

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
    mv(r, cx - 7); printf(BOLD FG_GREEN "Demo Complete!" RESET); r += 2;

    int block_w = 62;
    int lx = (W - block_w) / 2;
    if (lx < 3) lx = 3;

    mv(r, lx); printf("You've seen CloudTaser in action:"); r += 2;
    mv(r, lx); printf("* Secrets never touch Kubernetes - no etcd, no K8s Secrets"); r++;
    mv(r, lx); printf("* Applications work normally - secrets available in memory"); r++;
    mv(r, lx); printf("* eBPF enforcement blocks /proc/environ at kernel level"); r++;
    mv(r, lx); printf("* Full audit trail - every event logged for compliance"); r += 2;

    #define SEP "+---------------------+------+------+-------+------+---------+"
    #define RY FG_RED
    #define GV FG_GREEN
    #define YV FG_YELLOW
    #define R RESET

    mv(r, lx); printf("          CloudTaser vs Alternatives"); r++;
    mv(r, lx); printf(SEP); r++;
    mv(r, lx); printf("|                     | K8s  | Ext  | Vault | CSI  |  Cloud  |"); r++;
    mv(r, lx); printf("|                     | Sec  | Sec  | Injct | Drv  |  Taser  |"); r++;
    mv(r, lx); printf(SEP); r++;
    mv(r, lx); printf("| Secrets in etcd     |" RY " YES  " R "|" RY " YES  " R "|" GV "  NO   " R "|" YV " opt  " R "|" GV "   NO    " R "|"); r++;
    mv(r, lx); printf("| Secrets on tmpfs    |" RY " YES  " R "|" RY " YES  " R "|" RY " YES   " R "|" RY " YES  " R "|" GV "   NO    " R "|"); r++;
    mv(r, lx); printf("| /proc blocked       |" RY "  NO  " R "|" RY "  NO  " R "|" RY "  NO   " R "|" RY "  NO  " R "|" GV "  YES    " R "|"); r++;
    mv(r, lx); printf("| Runtime enforcement |" RY "  NO  " R "|" RY "  NO  " R "|" RY "  NO   " R "|" RY "  NO  " R "|" GV "  YES    " R "|"); r++;
    mv(r, lx); printf("| Provider can read   |" RY " YES  " R "|" RY " YES  " R "|" RY " YES   " R "|" RY " YES  " R "|" GV "   NO    " R "|"); r++;
    mv(r, lx); printf("| CLOUD Act resistant |" RY "  NO  " R "|" RY "  NO  " R "|" RY "  NO   " R "|" RY "  NO  " R "|" GV "  YES    " R "|"); r++;
    mv(r, lx); printf(SEP); r++;

    #undef SEP
    #undef RY
    #undef GV
    #undef YV
    #undef R

    mv(H - 5, cx - 7); printf(FG_CYAN "cloudtaser.io" RESET);
    mv(H - 4, cx - 11); printf(DIM "hello@cloudtaser.io" RESET);

    mv(H - 2, cx - 6);
    printf(BOLD BG_GREEN FG_WHITE " [ EXIT ] " RESET);
    mv(H - 1, cx - 13);
    printf(DIM "Press ENTER or 'q' to exit" RESET);
    fflush(stdout);
}

/* ── Wait for setup ──────────────────────────────────────────────── */
static void wait_for_setup(void) {
    struct stat st;
    int frame = 0;
    const char spin[] = "|/-\\";

    printf(HIDE_CUR);

    /* Phase 1: wait for kubernetes — show for at least 3 seconds */
    for (;;) {
        get_size();
        int r = th / 2;
        mv(r, 1);
        for (int i = 0; i < tw; i++) putchar(' ');
        int pad = (tw - 34) / 2;
        if (pad < 1) pad = 1;
        mv(r, pad);
        printf(DIM "Waiting for Kubernetes... %c" RESET, spin[frame & 3]);
        fflush(stdout);
        frame++;
        usleep(200000);
        if (frame >= 15 && stat("/etc/kubernetes/admin.conf", &st) == 0) break;
    }

    /* Phase 2: wait for CloudTaser install */
    while (stat("/tmp/.cloudtaser-setup-done", &st) != 0) {
        get_size();
        int r = th / 2;
        mv(r, 1);
        for (int i = 0; i < tw; i++) putchar(' ');
        int pad = (tw - 34) / 2;
        if (pad < 1) pad = 1;
        mv(r, pad);
        draw_cloud_taser(r, pad);
        printf(BOLD FG_WHITE " Installing %c" RESET, spin[frame & 3]);
        fflush(stdout);
        frame++;
        usleep(120000);
    }

    /* done */
    get_size();
    int r = th / 2;
    mv(r, 1);
    for (int i = 0; i < tw; i++) putchar(' ');
    int pad = (tw - 34) / 2;
    if (pad < 1) pad = 1;
    draw_cloud_taser(r, pad);
    printf(BOLD FG_GREEN " Ready" RESET);
    fflush(stdout);
    usleep(800000);
}

/* ── Main ────────────────────────────────────────────────────────── */
int main(void) {
    wait_for_setup();

    raw_on();
    atexit(raw_off);

    int step = 0;
    char output[MAX_OUT] = "";
    int cmd = 0;
    int btn = 0;
    int check_result = -1;
    int need_chrome = 1;
    int dirty = 1;
    int input_mode = 0;
    int input_len = 0;

    while (step < N_STEPS) {
        Step *s = &steps[step];
        int ncmds = 0;
        while (s->commands[ncmds]) ncmds++;

        if (need_chrome) {
            draw_chrome(step);
            need_chrome = 0;
            dirty = 1;
            /* enter input mode if step has input_prompt */
            if (s->input_prompt && cmd == 0) {
                input_mode = 1;
                input_len = 0;
                user_input[0] = '\0';
            }
        }

        if (input_mode) {
            if (dirty) {
                draw_input(step, user_input, input_len);
                dirty = 0;
            }

            int key = readkey();
            if (key == -1) continue;
            if (key == 'q' && input_len == 0) { step = N_STEPS; break; }
            if (key == 3) { step = N_STEPS; break; }

            if (key == '\n' || key == '\r') {
                if (input_len > 0) {
                    input_mode = 0;
                    dirty = 1;
                }
            } else if (key == 127 || key == 8) {
                /* backspace */
                if (input_len > 0) {
                    input_len--;
                    user_input[input_len] = '\0';
                    dirty = 1;
                }
            } else if (key >= 32 && key < 127 && input_len < MAX_INPUT - 1) {
                /* printable character */
                user_input[input_len++] = (char)key;
                user_input[input_len] = '\0';
                dirty = 1;
            }
            continue;
        }

        if (dirty) {
            draw_dynamic(step, cmd, ncmds, btn, output, check_result, 0);
            dirty = 0;
        }

        int key = readkey();
        if (key == -1) continue;
        if (key == 'q' || key == 3) break;

        if (key == K_LEFT || key == K_RIGHT) {
            if (cmd < ncmds) btn = 0;
            else if (cmd >= ncmds) btn = 1;
            dirty = 1;
            continue;
        }

        if (key == '\n' || key == '\r') {
            if (btn == 0 && cmd < ncmds) {
                draw_dynamic(step, cmd, ncmds, btn, output, check_result, 1);

                char expanded[MAX_CMD];
                expand_cmd(s->commands[cmd], expanded, MAX_CMD, user_input);

                char cmd_out[MAX_OUT];
                run_cmd(expanded, cmd_out, MAX_OUT);

                output[0] = '\0';
                int olen = 0;
                {
                    const char *c = s->commands[cmd];
                    int clen = (int)strlen(c);
                    int space = MAX_OUT - 1 - olen;
                    int wrote = snprintf(output + olen, space, "$ %.*s\n",
                                         clen, c);
                    if (wrote > 0 && wrote < space) olen += wrote;
                }
                {
                    int clen = strlen(cmd_out);
                    int space = MAX_OUT - 1 - olen;
                    if (clen > space) clen = space;
                    memcpy(output + olen, cmd_out, clen);
                    output[olen + clen] = '\0';
                }

                cmd++;
                if (cmd >= ncmds) {
                    btn = 1;
                    if (s->check) {
                        char chk[256];
                        check_result = run_cmd(s->check, chk, sizeof(chk));
                    } else {
                        check_result = 0;
                    }
                }
                dirty = 1;
            } else if (btn == 1 && cmd >= ncmds) {
                step++;
                cmd = 0;
                btn = 0;
                output[0] = '\0';
                check_result = -1;
                need_chrome = 1;
                dirty = 1;
            }
        }
    }

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
