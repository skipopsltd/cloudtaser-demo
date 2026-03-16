# Demo Complete

You've seen CloudTaser's S3 Proxy in action:

- **Client-side encryption** — data is encrypted before it reaches the cloud
- **Transparent to applications** — standard S3 API, no code changes
- **Per-object unique keys** — unique AES-256-GCM key for every object
- **EU key sovereignty** — encryption keys wrapped by EU-hosted Vault Transit

## What's different from server-side encryption?

| | CloudTaser S3 Proxy | AWS SSE-S3 | AWS SSE-KMS | AWS SSE-C |
|---|---|---|---|---|
| Provider sees plaintext | Never | Always | Always | Briefly |
| Key held by | EU Vault | AWS | AWS KMS | You (but sent to AWS) |
| CLOUD Act exposure | Ciphertext only | Full plaintext | Full plaintext | Key in transit |
| Code changes needed | Endpoint URL only | None | Minor | Major |
| Per-object unique keys | Yes | Yes | Optional | Manual |

## Next Steps

Ready to protect your data sovereignty?

**Learn more:** [cloudtaser.io](https://cloudtaser.io)

**Contact us:** cloud@skipops.ltd
