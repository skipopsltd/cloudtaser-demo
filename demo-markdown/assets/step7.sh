#!/bin/bash
# Step 7: eBPF blocks /proc/environ, show audit trail
source /tmp/helpers.sh
step_guard 7

header "Step 7/8: eBPF Runtime Enforcement"

info "In step 3 we read the password from /proc/environ — anyone with host access could."
info "Now CloudTaser's eBPF kprobe blocks this at kernel level — even for root."

section "Find the protected process PID"

run_cmd "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 \\
  | grep -o '\"host_pid\":[0-9]*' | tail -1 | cut -d: -f2 \\
  | tee /tmp/.protected_pid \\
  | xargs -I{} echo \"Protected PID: {}\""

pause

section "Try to read /proc/PID/environ as root"

PROTECTED_PID=$(cat /tmp/.protected_pid)

run_cmd "sudo cat /proc/${PROTECTED_PID}/environ 2>&1; echo \"Exit code: \$?\""

info "Permission denied — even as root."
info "The eBPF kprobe returned -EACCES before any data was read."
info "Compare this to step 3, where we could freely read the password."

pause

section "Audit trail"

run_cmd "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 \\
  | grep -E \"ENVIRON|blocked\""

info "Every access attempt is logged with PID, command, timestamp, and severity."
info "Events forward to SIEM for GDPR/NIS2/DORA compliance."

pause
