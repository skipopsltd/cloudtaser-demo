# CloudTaser

EU/UK data sovereignty on US cloud infrastructure. Secrets never touch Kubernetes — fetched from EU vault directly into process memory, enforced by eBPF.

## What this demo covers

1. Install CloudTaser operator and eBPF daemonset
2. Deploy PostgreSQL the **traditional way** — K8s Secret + env vars
3. See the problem: your password is visible in etcd, env, /proc
4. Move the password to an EU vault, delete the K8s Secret
5. **Redeploy with CloudTaser** — no secrets in the manifest
6. Verify: secrets are gone from K8s, but the app still works
7. Watch eBPF block a root-level `/proc/environ` read
8. Compare CloudTaser to alternatives

Each step tells you to run a short bash script. The script shows every command before running it — no magic.

**The environment takes 1–2 minutes to set up.** Wait for the terminal to be ready before proceeding.
