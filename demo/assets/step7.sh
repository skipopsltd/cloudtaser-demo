#!/bin/bash
# Step 7: eBPF blocks /proc/environ, /proc/mem, ptrace — show audit trail
source /tmp/helpers.sh
step_guard 7

header "Step 7/8: eBPF Runtime Enforcement"

info "In step 3 we could read the password from /proc. Now eBPF blocks it — even as root."

section "Find protected process PID"

run_cmd "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 \\
  | grep -o '\"host_pid\":[0-9]*' | tail -1 | cut -d: -f2 \\
  | tee /tmp/.protected_pid \\
  | xargs -I{} echo \"Protected PID: {}\""

PROTECTED_PID=$(cat /tmp/.protected_pid)

section "1. /proc/PID/environ — blocked"

run_cmd "sudo cat /proc/${PROTECTED_PID}/environ 2>&1; echo \"Exit code: \$?\""

section "2. /proc/PID/mem — blocked"

run_cmd "sudo cat /proc/${PROTECTED_PID}/mem 2>&1; echo \"Exit code: \$?\""

section "3. ptrace attach — blocked"

run_cmd "sudo strace -p ${PROTECTED_PID} -e trace=none -o /dev/null 2>&1; echo \"Exit code: \$?\""

info "All three blocked. eBPF kprobes return -EACCES/-EPERM at kernel level."

pause

section "Audit trail"

run_cmd "kubectl logs -n cloudtaser-system ds/cloudtaser-ebpf --tail=50 \\
  | grep -E \"ENVIRON|PROCMEM|PTRACE|blocked\""

info "Every attempt is logged. Events forward to SIEM for GDPR/NIS2/DORA compliance."

pause
