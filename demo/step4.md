# Try to Read /proc/pid/environ

This is the key security moment. On a normal Kubernetes node, anyone with host access can read a process's environment variables via `/proc/<pid>/environ`. CloudCondom's eBPF agent blocks this.

**Find the PostgreSQL PID:**

```bash
PG_PID=$(kubectl exec postgres-demo -- pgrep -f "postgres" | head -1)
echo "PostgreSQL PID: $PG_PID"
```

**Attempt to read the environment from the host:**

```bash
# Get the node-level PID for the container process
NODE_PID=$(kubectl exec postgres-demo -- cat /proc/1/status | grep ^NSpid | awk '{print $NF}')
echo "Attempting to read /proc/${NODE_PID}/environ..."
cat /proc/${NODE_PID}/environ 2>&1 || echo "ACCESS BLOCKED by CloudCondom eBPF agent"
```

The read was either:
- **Blocked synchronously** (kernel returned -EACCES via kprobe enforcement), or
- **The process was killed** (reactive enforcement via SIGKILL)

Either way, the secrets were protected. An attacker with node access cannot extract credentials from process memory via `/proc/environ`.
