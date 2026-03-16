# CloudTaser — Live Demo

CloudTaser enables EU companies to use US cloud providers (AWS, GCP, Azure) with cryptographic guarantees that neither the provider nor US government can access your data.

## What you'll see

In this 10-minute demo, you'll:

1. **Deploy** a PostgreSQL pod with CloudTaser protection
2. **Unseal** the wrapper by delivering a Vault token (like Vault's unseal flow)
3. **Verify** that secrets never appear in Kubernetes Secrets or etcd
4. **Confirm** that your application has full access to its credentials
5. **Witness** CloudTaser blocking an attempt to read `/proc/pid/environ`
6. **Review** the audit trail of the blocked access attempt

## How it works

CloudTaser's mutating webhook intercepts pod creation and injects a thin wrapper that:
- Starts in **sealed mode** — alive but waiting for a Vault token
- Once unsealed, fetches secrets directly from an **EU-hosted Vault** (Frankfurt) into process memory
- Secrets exist only in memory — never in etcd, K8s Secrets, or on disk

An eBPF daemonset on every node monitors for secret leaks and blocks unauthorized access to `/proc/pid/environ`.

## Environment

This demo connects to:
- **EU Vault** at `secret.cloudtaser.io` — hosted in Frankfurt (GCP europe-west3)
- **CloudTaser** operator, wrapper, and eBPF agent pre-installed via Helm
- A scoped session token (1h TTL) is automatically provisioned

Setup takes about 2 minutes. Step 1 includes a command to wait for setup completion.
