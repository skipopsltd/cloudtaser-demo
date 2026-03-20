#!/bin/bash
# Step 2: Upload through CloudTaser S3 Proxy
source /tmp/helpers.sh
step_guard 2

header "Step 2/5: Upload Through CloudTaser S3 Proxy"

source /tmp/.demo-env

info "The S3 proxy encrypts data before it leaves your application."
info "The cloud provider (MinIO on play.min.io) stores only ciphertext."

section "Upload through the proxy"

run_cmd "mc cp /tmp/confidential-report.txt proxy/\${BUCKET}/confidential-report.txt"

section "Verify it's in the bucket"

run_cmd "mc ls proxy/\${BUCKET}/"

info "File uploaded. The proxy encrypted it with a key from the EU vault before sending."

pause
