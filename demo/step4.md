# Try to Read /proc/pid/environ

This is the key security moment. On a normal Kubernetes node, anyone with host access can read a process's environment variables via `/proc/<pid>/environ`. CloudTaser's eBPF agent detects every such attempt.

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

**Attempt to read the environment from the host:**

```bash
cat /proc/${PROTECTED_PID}/environ 2>&1; echo "Exit code: $?"
```

The eBPF agent **detected the access and killed the reader** (reactive enforcement via SIGKILL). On kernels with kprobe support (most production clusters), the `openat` syscall is blocked *before* any data is returned — the read gets `-EACCES`. On this demo kernel, the reactive kill fires after the read completes, but the access is still logged and the process terminated.

Either way, the access attempt is **recorded in the audit trail** — which is what compliance frameworks (GDPR, NIS2, DORA) require.
