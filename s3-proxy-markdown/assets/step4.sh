#!/bin/bash
# Step 4: Download through proxy — decrypted
source /tmp/helpers.sh
step_guard 4

header "Step 4/5: Download Through the Proxy"

source /tmp/.demo-env

info "Now download through the proxy — it decrypts transparently."

section "Download through proxy"

run_cmd "mc cp proxy/\${BUCKET}/confidential-report.txt /tmp/proxy-copy.txt"

section "Decrypted content"

run_cmd "cat /tmp/proxy-copy.txt"

section "Compare with original"

run_cmd "diff /tmp/confidential-report.txt /tmp/proxy-copy.txt && echo 'Files are identical.'"

info "Same file. Encrypted at rest, decrypted on read — transparent to the application."

pause
