# What Your Application Sees

Now read the same object through the CloudTaser proxy — exactly as your application would:

```bash
aws --endpoint-url http://localhost:8190 s3 cp \
  s3://$BUCKET/reports/q1-2026.txt /tmp/decrypted.txt

cat /tmp/decrypted.txt
```

The proxy transparently:
1. Downloaded the ciphertext from the cloud
2. Unwrapped the data encryption key via the EU Vault Transit engine
3. Decrypted the object using AES-256-GCM
4. Returned the plaintext to your application

**No code changes required** — your application uses the standard S3 API. Just point it at `localhost:8190` instead of the cloud endpoint.

**Verify the decryption is lossless:**

```bash
diff /tmp/confidential-report.txt /tmp/decrypted.txt && echo "Files are identical"
```
