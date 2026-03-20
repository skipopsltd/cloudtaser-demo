# CloudTaser S3 Proxy

Client-side encryption for S3-compatible object storage. Data is encrypted before it leaves your application — the cloud provider stores only ciphertext. Encryption keys stay in an EU-hosted vault.

## What this demo covers

1. View a confidential file with GDPR-sensitive data
2. Upload it through the CloudTaser S3 Proxy (encrypted automatically)
3. Read it directly from the cloud — ciphertext only
4. Download through the proxy — decrypted transparently
5. Compare with alternatives

Each step tells you to run a short bash script. The script shows every command before running it.

**The environment takes 1–2 minutes to set up.** Wait for the terminal to be ready before proceeding.
