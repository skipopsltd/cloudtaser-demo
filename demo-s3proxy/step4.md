# The Encryption Envelope

Each object gets its own unique encryption key. Let's examine the metadata:

```bash
aws --endpoint-url https://play.min.io s3api head-object \
  --bucket $BUCKET --key reports/q1-2026.txt \
  --query 'Metadata' --output json | python3 -m json.tool
```

| Header | Purpose |
|--------|---------|
| `cloudtaser-dek` | Data Encryption Key, wrapped by Vault Transit (useless without EU Vault) |
| `cloudtaser-nonce` | Unique nonce for AES-256-GCM |
| `cloudtaser-algorithm` | `AES-256-GCM` — authenticated encryption with integrity verification |
| `cloudtaser-key-name` | Vault Transit key that wrapped the DEK |
| `cloudtaser-key-version` | Key version (supports seamless key rotation) |
| `cloudtaser-plaintext-size` | Original file size |

**Upload a second document to prove per-object key uniqueness:**

```bash
echo "Another confidential document - board meeting notes" > /tmp/second-doc.txt
aws --endpoint-url http://localhost:8190 s3 cp \
  /tmp/second-doc.txt s3://$BUCKET/reports/board-notes.txt
```

**Compare the wrapped DEKs — each object has a different encryption key:**

```bash
echo "=== Document 1 DEK ==="
aws --endpoint-url https://play.min.io s3api head-object \
  --bucket $BUCKET --key reports/q1-2026.txt \
  --query 'Metadata."cloudtaser-dek"' --output text | cut -c1-60
echo "..."

echo "=== Document 2 DEK ==="
aws --endpoint-url https://play.min.io s3api head-object \
  --bucket $BUCKET --key reports/board-notes.txt \
  --query 'Metadata."cloudtaser-dek"' --output text | cut -c1-60
echo "..."
```

Different DEK for every object — compromise of one key cannot decrypt other objects.

**Why this matters for EU data sovereignty:**

The cloud provider holds the ciphertext and the wrapped DEKs. But the wrapped DEKs can **only be unwrapped by the Vault Transit engine** at `secret.cloudtaser.io` — hosted in Frankfurt (GCP europe-west3), under EU jurisdiction. Even with a US government subpoena (CLOUD Act, FISA Section 702), the cloud provider can only hand over ciphertext and wrapped keys — both useless without access to the EU-hosted Transit engine.
