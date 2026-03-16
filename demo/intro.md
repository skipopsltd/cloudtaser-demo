# CloudTaser — Live Demo

CloudTaser enables EU companies to use US cloud providers (AWS, GCP, Azure) with cryptographic guarantees that neither the provider nor US government can access your data.

## What you'll see

In this 10-minute demo, you'll:

1. **Deploy** a PostgreSQL pod with CloudTaser protection — the operator auto-unseals it
2. **Verify** that secrets never appear in Kubernetes Secrets or etcd
3. **Confirm** that your application has full access to its credentials
4. **Witness** CloudTaser blocking an attempt to read `/proc/pid/environ`
5. **Review** the audit trail of the blocked access attempt

## How it works

CloudTaser's mutating webhook intercepts pod creation and injects a thin wrapper. The operator acts as an **auth broker**:
- It holds a single EU Vault token, delivered once during setup
- When a new pod is created, the operator creates a scoped child token and delivers it to the wrapper
- The wrapper fetches secrets directly from an **EU-hosted Vault** (Frankfurt) into process memory
- Secrets exist only in memory — never in etcd, K8s Secrets, or on disk

An eBPF daemonset on every node monitors for secret leaks and blocks unauthorized access to `/proc/pid/environ`.

## Environment

This demo connects to:
- **EU Vault** at `secret.cloudtaser.io` — hosted in Frankfurt (GCP europe-west3)
- **CloudTaser** operator, wrapper, and eBPF agent pre-installed via Helm
- The operator is pre-unsealed with a scoped session token (1h TTL)

Setup takes about 2 minutes. Step 1 includes a command to wait for setup completion.
