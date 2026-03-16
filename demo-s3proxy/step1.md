# Upload a Confidential Document

Load the demo environment:

```bash
source /tmp/.demo-env
echo "Bucket: $BUCKET"
```

Here's the confidential EU financial report we'll upload:

```bash
cat /tmp/confidential-report.txt
```

Now upload it through the CloudTaser S3 Proxy. We point at `localhost:8190` — the proxy encrypts the data locally and forwards only ciphertext to the cloud:

```bash
aws --endpoint-url http://localhost:8190 s3 cp \
  /tmp/confidential-report.txt s3://$BUCKET/reports/q1-2026.txt
```

The document has been uploaded. But what did the cloud provider actually receive?
