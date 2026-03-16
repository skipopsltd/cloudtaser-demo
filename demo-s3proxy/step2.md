# What the Cloud Provider Sees

Let's bypass the CloudTaser proxy and read the object **directly from the cloud** (play.min.io):

```bash
aws --endpoint-url https://play.min.io s3 cp \
  s3://$BUCKET/reports/q1-2026.txt /tmp/cloud-raw.bin
```

**Compare the raw bytes stored in the cloud vs the original document:**

```bash
echo "=== Original document (first 5 lines) ==="
head -5 /tmp/confidential-report.txt

echo ""
echo "=== What the cloud provider stores ==="
xxd /tmp/cloud-raw.bin | head -16

echo ""
echo "Original size:  $(wc -c < /tmp/confidential-report.txt) bytes"
echo "Encrypted size: $(wc -c < /tmp/cloud-raw.bin) bytes"
```

The cloud provider sees **random bytes** — AES-256-GCM ciphertext. No financial data, no customer names, no salary information. The slight size increase is the GCM authentication tag.

Now look at the metadata the cloud provider can see:

```bash
aws --endpoint-url https://play.min.io s3api head-object \
  --bucket $BUCKET --key reports/q1-2026.txt \
  --query 'Metadata' --output table
```

The cloud stores encryption metadata alongside the object, but the data encryption key is **wrapped by the EU Vault Transit engine** — it cannot be unwrapped without EU Vault access.
