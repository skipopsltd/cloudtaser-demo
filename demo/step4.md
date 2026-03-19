# Try to Read /proc/pid/environ

This is the key security moment. On a normal Kubernetes node, even root can freely read a process's environment variables via `/proc/<pid>/environ`. CloudTaser's eBPF agent **blocks this at the kernel level — even for root**.

**Find the protected process's host PID:**

The eBPF agent tracks the exact PID it's protecting. Let's get it:

```bash
PROTECTED_PID=$(kubectl logs -n cloudtaser-system daemonset/cloudtaser-ebpf --tail=50 | python3 -c "
import sys, json
for line in sys.stdin:
    try:
        evt = json.loads(line.strip())
        if 'registered PID' in evt.get('msg', ''):
            pid = evt.get('host_pid', '')
            if pid: print(pid)
    except: pass
" | tail -1)
echo "Protected host PID: $PROTECTED_PID"
```

**Attempt to read the environment as root:**

```bash
sudo cat /proc/${PROTECTED_PID}/environ 2>&1; echo "Exit code: $?"
```

You should see **"Permission denied"** — even as root. The eBPF kprobe intercepted the `openat` syscall and returned `-EACCES` *before any data was read*. This is synchronous kernel-level enforcement, not a race condition.

The access attempt is **recorded in the audit trail** — which is what compliance frameworks (GDPR, NIS2, DORA) require.
