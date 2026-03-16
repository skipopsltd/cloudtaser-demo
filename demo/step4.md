# Try to Read /proc/pid/environ

This is the key security moment. On a normal Kubernetes node, anyone with host access can read a process's environment variables via `/proc/<pid>/environ`. CloudTaser's eBPF agent detects and blocks this.

**Find the application's host PID:**

The wrapper (PID 1 in container) fork+execs the real application as a child process. We need the child's host PID — that's the process with secrets in its environment.

```bash
# Get the container ID and wrapper host PID
CID=$(kubectl get pod postgres-demo -o jsonpath='{.status.containerStatuses[0].containerID}' | sed 's|containerd://||')
WRAPPER_PID=$(crictl inspect "$CID" 2>/dev/null | python3 -c "import sys,json; print(json.load(sys.stdin)['info']['pid'])")

# Find the child process (the actual postgres process with secrets)
CHILD_PID=$(cat /proc/${WRAPPER_PID}/task/${WRAPPER_PID}/children 2>/dev/null | awk '{print $1}')
echo "Wrapper host PID: $WRAPPER_PID"
echo "Child (postgres) host PID: $CHILD_PID"
```

**Attempt to read the environment from the host:**

```bash
echo "Attempting to read /proc/${CHILD_PID}/environ..."
cat /proc/${CHILD_PID}/environ 2>&1 || echo "ACCESS BLOCKED by CloudTaser eBPF agent"
```

The `cat` process was **killed by the eBPF agent** (reactive enforcement via SIGKILL). The secrets were protected — an attacker with node access cannot extract credentials from process memory via `/proc/environ`.
