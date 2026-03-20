#!/bin/bash
# Step 3: Read directly from the cloud — ciphertext
source /tmp/helpers.sh
step_guard 3

header "Step 3/5: What the Cloud Provider Sees"

source /tmp/.demo-env

info "Let's bypass the proxy and read the file directly from the cloud."

section "Download directly from MinIO (bypassing proxy)"

run_cmd "mc cat cloud/\${BUCKET}/confidential-report.txt > /tmp/cloud-copy.txt"

section "What's in the file?"

run_cmd "cat /tmp/cloud-copy.txt 2>/dev/null || echo '(binary/encrypted content)'"

section "Raw bytes"

run_cmd "xxd /tmp/cloud-copy.txt | head -20"

info "Ciphertext. The cloud provider stores this — they can never read your data."
info "The encryption key stays in the EU vault. Never shared with the provider."

pause
