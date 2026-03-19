# CloudTaser

EU/UK data sovereignty on US cloud infrastructure. Secrets never touch Kubernetes — fetched from EU vault directly into process memory, enforced by eBPF.

## What this demo covers

1. Install CloudTaser operator and eBPF daemonset
2. Deploy a PostgreSQL pod — no secrets in the manifest
3. Set your own password in the EU vault (Frankfurt)
4. Inspect how the wrapper injection works
5. Verify: no secrets in K8s Secrets, etcd, or env vars
6. Watch eBPF block a root-level `/proc/environ` read
7. Compare CloudTaser to alternatives

Each step tells you to run a short bash script. The script shows every command before running it — no magic.

**The environment takes 1–2 minutes to set up.** Wait for the terminal to be ready before proceeding.
