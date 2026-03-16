# CloudTaser S3 Proxy — Live Demo

EU data sovereignty for object storage. CloudTaser's S3 proxy encrypts every object **before it leaves your application** — the cloud provider stores only ciphertext.

## What you'll see

1. **Upload** a confidential document through the CloudTaser proxy
2. **Inspect** what the cloud provider actually stores (ciphertext)
3. **Download** through the proxy — transparent decryption
4. **Examine** the envelope encryption metadata

## How it works

CloudTaser's S3 proxy runs as a sidecar in your Kubernetes pods. Applications point their S3 client at `localhost:8190` instead of the cloud endpoint. The proxy:

- Generates a unique **AES-256-GCM** data encryption key per object
- Encrypts the object locally before upload
- Wraps the encryption key via an **EU-hosted Vault Transit engine** (Frankfurt, Germany)
- Forwards only ciphertext to the cloud provider

The cloud provider never holds the encryption keys. Even with a US government subpoena (CLOUD Act, FISA 702), the provider can only hand over ciphertext.

## Environment

This demo connects to:
- **EU Vault** at `secret.cloudtaser.io` — Transit encryption engine hosted in Frankfurt (europe-west3)
- **CloudTaser S3 Proxy** — client-side encryption, running locally
- **MinIO play server** — simulating US cloud object storage

A scoped session token (1h TTL) is automatically provisioned for this demo session.

Setup takes about 1 minute.
