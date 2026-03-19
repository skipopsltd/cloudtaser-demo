# Interactive Demo

**👉 All the action happens in the terminal panel on the right.**

If you see a setup script still running, wait for it to finish. Once ready, the terminal shows an interactive demo menu.

## How to use

- Press **ENTER** to run each command
- Use **arrow keys** to navigate between buttons
- The demo guides you through 5 steps automatically
- At the end you can explore the cluster freely

## What you'll see

1. **Deploy** a protected PostgreSQL pod
2. **Set your own secret password** — stored in EU vault (Frankfurt)
3. **Verify** it's not in Kubernetes (not in etcd, not in pod spec, not on disk)
4. **Watch eBPF block** a `/proc/environ` read attempt
5. **View the audit trail** — every access attempt logged

**Complete the full demo in the terminal before clicking Next here.**
