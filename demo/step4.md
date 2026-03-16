# Try to Read /proc/pid/environ

This is the key security moment. On a normal Kubernetes node, anyone with host access can read a process's environment variables via `/proc/<pid>/environ`. CloudTaser's eBPF agent blocks this.

**Find the PostgreSQL container PID on the host:**

```bash
# Get the container ID
CID=$(kubectl get pod postgres-demo -o jsonpath='{.status.containerStatuses[0].containerID}' | sed 's|containerd://||')

# Find the host-level PID for the container's init process
HOST_PID=$(crictl inspect "$CID" 2>/dev/null | python3 -c "import sys,json; print(json.load(sys.stdin)['info']['pid'])")
echo "Container host PID: $HOST_PID"
```

**Attempt to read the environment from the host:**

```bash
echo "Attempting to read /proc/${HOST_PID}/environ..."
cat /proc/${HOST_PID}/environ 2>&1 || echo "ACCESS BLOCKED by CloudTaser eBPF agent"
```

The read was either:
- **Blocked synchronously** (kernel returned -EACCES via kprobe enforcement), or
- **The process was killed** (reactive enforcement via SIGKILL)

Either way, the secrets were protected. An attacker with node access cannot extract credentials from process memory via `/proc/environ`.
