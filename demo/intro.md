# CloudTaser / Phantom — Live Demo

CloudTaser enables EU companies to use US cloud providers (AWS, GCP, Azure) with cryptographic guarantees that neither the provider nor US government can access your data.

## What you'll see

In this 10-minute demo, you'll:

1. **Deploy** a PostgreSQL pod with CloudTaser protection
2. **Verify** that secrets never appear in Kubernetes Secrets or etcd
3. **Confirm** that your application has full access to its credentials
4. **Witness** CloudTaser blocking an attempt to read `/proc/pid/environ`
5. **Review** the audit trail of the blocked access attempt

## How it works

CloudTaser's mutating webhook intercepts pod creation and injects a thin wrapper that:
- Fetches secrets directly from an EU-hosted vault into process memory
- Rewrites the container entrypoint so the wrapper runs as PID 1
- Secrets exist only in memory — never in etcd, K8s Secrets, or on disk

An eBPF daemonset on every node monitors for secret leaks and blocks unauthorized access.

## Environment

This environment has a Kubernetes cluster with:
- Vault (dev mode) pre-configured with a PostgreSQL password
- CloudTaser operator, wrapper, and eBPF agent pre-installed via Helm
