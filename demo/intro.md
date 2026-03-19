# CloudTaser

EU/UK data sovereignty on US cloud infrastructure. Secrets never touch Kubernetes — fetched from EU vault directly into process memory, enforced by eBPF.

## What this demo does

1. Deploy a PostgreSQL pod with CloudTaser annotations — no secrets in the YAML
2. Operator webhook auto-injects the CloudTaser wrapper into the pod
3. Wrapper fetches secrets from an EU-hosted vault directly into process memory
4. Set your own password in the EU vault — the pod picks it up on restart
5. Verify: no secrets in K8s Secrets, etcd, env vars, or on disk
6. eBPF blocks `/proc/environ` reads at kernel level — even with host access
7. View the audit trail — every access attempt is logged for compliance

## ⚠️ Before you click Start

The environment takes **1–2 minutes** to prepare after you press Start. A setup script deploys CloudTaser to the cluster and configures the connection to the EU vault.

**Look at the terminal on the right** — wait until you see the interactive demo menu before proceeding. Do not click through the steps in this panel until the terminal is ready.
